#!/usr/bin/env python3
"""Regenerate the checked-in x86_64 kext project. No build or installation."""
from pathlib import Path
import hashlib
import json
import plistlib

root = Path(__file__).resolve().parent.parent
module_version = plistlib.loads((root / 'kext/Info.plist').read_bytes())['CFBundleVersion']
sources = sorted(str(p.relative_to(root)) for p in (root / 'src').glob('*.c'))
b14=json.loads((root/'src/RTW8814BHAL-Upstream.json').read_text())
sources += ['src/rtl8814b/halmac/'+name for name in b14['sources']]
sources += ['kext/RTWUSBController.cpp', 'kext/RTWUserClient.cpp',
            'kext/RTW8192EUHAL.cpp', 'kext/RTWNet80211Globals.cpp', 'vendor/rtwn/rtwn.cpp']
sources += ['vendor/itl80211/' + p for p in json.loads((root / 'vendor/itl80211/sources.json').read_text())]
headers = sorted(str(p.relative_to(root)) for folder in ['include', 'kext']
                 for p in (root / folder).rglob('*') if p.suffix in ['.h', '.hpp', '.inc'])

def uid(value):
    return hashlib.sha256(value.encode()).hexdigest()[:24].upper()

def quote(s):
    return json.dumps(s)

objects = []
refs, builds = [], []
for path in sources + headers:
    file_id = uid('file:' + path)
    kind = 'sourcecode.cpp.cpp' if path in sources and (path.startswith('vendor/') or path.endswith('.cpp')) else ('sourcecode.c.c' if path.endswith('.c') else 'sourcecode.c.h')
    objects.append(f'{file_id} = {{isa=PBXFileReference; lastKnownFileType={kind}; path={quote(path)}; sourceTree=SOURCE_ROOT;}};')
    refs.append(file_id)
    if path in sources:
        build_id = uid('build:' + path)
        flags = ''
        if kind == 'sourcecode.cpp.cpp':
            flags = ' settings={COMPILER_FLAGS=' + quote('-include "$(PROJECT_DIR)/vendor/itl80211/RTWNet80211Prefix.hpp"') + ';};'
        objects.append(f'{build_id} = {{isa=PBXBuildFile; fileRef={file_id};{flags}}};')
        builds.append(build_id)

target = 'BB0000000000000000000015'  # Preserve the shared scheme identity.
project = 'BB0000000000000000000016'
product, group, products, phase, linkage, signing = map(uid, ['product', 'main-group', 'products', 'sources-phase', 'linkage-phase', 'adhoc-signing-phase'])
linkage_script = '/usr/bin/python3 "$PROJECT_DIR/scripts/check-kext-linkage.py" "$TARGET_BUILD_DIR/$EXECUTABLE_PATH"\n'
signing_script = (
    'set -euo pipefail\n'
    'source "$PROJECT_DIR/app/signing.sh"\n'
    'rtw_sign_kext "$TARGET_BUILD_DIR/$FULL_PRODUCT_NAME"\n'
)
objects += [
    f'{product} = {{isa=PBXFileReference; explicitFileType=wrapper.kext; path=TahoeRealtekWiFi.kext; sourceTree=BUILT_PRODUCTS_DIR;}};',
    f'{products} = {{isa=PBXGroup; children=({product},); name=Products; sourceTree="<group>";}};',
    f'{group} = {{isa=PBXGroup; children=({",".join(refs)},{products},); sourceTree="<group>";}};',
    f'{phase} = {{isa=PBXSourcesBuildPhase; buildActionMask=2147483647; files=({",".join(builds)},); runOnlyForDeploymentPostprocessing=0;}};',
    f'{linkage} = {{isa=PBXShellScriptBuildPhase; buildActionMask=2147483647; files=(); inputPaths=("$(TARGET_BUILD_DIR)/$(EXECUTABLE_PATH)",); outputPaths=(); alwaysOutOfDate=1; name="Check driver linkage"; runOnlyForDeploymentPostprocessing=0; shellPath=/bin/sh; shellScript={quote(linkage_script)};}};',
    f'{signing} = {{isa=PBXShellScriptBuildPhase; buildActionMask=2147483647; files=(); inputPaths=("$(TARGET_BUILD_DIR)/$(EXECUTABLE_PATH)","$(TARGET_BUILD_DIR)/$(INFOPLIST_PATH)","$(PROJECT_DIR)/app/signing.sh",); outputPaths=(); alwaysOutOfDate=1; name="Require ad-hoc signature"; runOnlyForDeploymentPostprocessing=0; shellPath=/bin/bash; shellScript={quote(signing_script)};}};',
    f'{target} = {{isa=PBXNativeTarget; buildConfigurationList={uid("target-configs")}; buildPhases=({phase},{linkage},{signing},); buildRules=(); dependencies=(); name=TahoeRealtekWiFi; productName=TahoeRealtekWiFi; productReference={product}; productType="com.apple.product-type.kernel-extension";}};',
    f'{project} = {{isa=PBXProject; attributes={{LastUpgradeCheck=2600;}}; buildConfigurationList={uid("project-configs")}; compatibilityVersion="Xcode 14.0"; developmentRegion=en; knownRegions=(en,Base); mainGroup={group}; productRefGroup={products}; projectDirPath=""; projectRoot=""; targets=({target},);}};',
]
project_configs, target_configs = [], []
for name in ['Debug', 'Release']:
    pc, tc = uid('project-' + name), uid('target-' + name)
    project_configs.append(pc)
    target_configs.append(tc)
    settings = {
        'SDKROOT': 'macosx', 'ARCHS': 'x86_64', 'ONLY_ACTIVE_ARCH': 'NO',
        'ALWAYS_SEARCH_USER_PATHS': 'NO', 'CLANG_CXX_LANGUAGE_STANDARD': 'gnu++14',
        'GCC_C_LANGUAGE_STANDARD': 'c11', 'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',
        'GCC_ENABLE_CPP_RTTI': 'NO', 'CLANG_ENABLE_MODULES': 'NO',
        'CODE_SIGNING_ALLOWED': 'NO', 'ENABLE_USER_SCRIPT_SANDBOXING': 'NO', 'MACOSX_DEPLOYMENT_TARGET': '11.0',
        'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES', 'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES',
        'KERNEL_EXTENSION_HEADER_SEARCH_PATHS': '$(PROJECT_DIR)/MacKernelSDK/Headers',
        'KERNEL_FRAMEWORK_HEADERS': '$(PROJECT_DIR)/MacKernelSDK/Headers',
        'GCC_OPTIMIZATION_LEVEL': '0' if name == 'Debug' else 's',
    }
    body = ''.join(k+'='+quote(v)+';' for k,v in settings.items())
    body += 'GCC_PREPROCESSOR_DEFINITIONS=("$(inherited)","KERNEL=1","KERNEL_PRIVATE=1","DRIVER_PRIVATE=1","IEEE80211_STA_ONLY=1","__PRIVATE_SPI__=1");'
    body += 'LIBRARY_SEARCH_PATHS=("$(inherited)","$(PROJECT_DIR)/MacKernelSDK/Library/x86_64");'
    body += 'HEADER_SEARCH_PATHS=("$(PROJECT_DIR)/vendor/itl80211/openbsd","$(PROJECT_DIR)/vendor/itl80211/linux","$(PROJECT_DIR)/vendor/itl80211","$(PROJECT_DIR)/include","$(PROJECT_DIR)/kext","$(PROJECT_DIR)/src/rtl8814b/halmac","$(PROJECT_DIR)/vendor/rtwn","$(inherited)");'
    body += 'OTHER_CPLUSPLUSFLAGS=("$(inherited)","-fno-builtin","-Wno-deprecated-register","-Wno-c++17-extensions","-Wno-macro-redefined","-Wno-format","-Werror=return-type");'
    objects.append(f'{pc}={{isa=XCBuildConfiguration;buildSettings={{{body}}};name={name};}};')
    body = 'PRODUCT_NAME=TahoeRealtekWiFi;PRODUCT_BUNDLE_IDENTIFIER=org.research.TahoeRealtekWiFi;MODULE_NAME=org.research.TahoeRealtekWiFi;MODULE_VERSION=' + quote(module_version) + ';INFOPLIST_FILE=kext/Info.plist;GENERATE_INFOPLIST_FILE=NO;WRAPPER_EXTENSION=kext;SKIP_INSTALL=YES;CODE_SIGNING_ALLOWED=NO;'
    objects.append(f'{tc}={{isa=XCBuildConfiguration;buildSettings={{{body}}};name={name};}};')
for label,configs in [('project-configs',project_configs),('target-configs',target_configs)]:
    objects.append(f'{uid(label)}={{isa=XCConfigurationList;buildConfigurations=({",".join(configs)},);defaultConfigurationIsVisible=0;defaultConfigurationName=Release;}};')
(root / 'TahoeRealtekWiFi.xcodeproj/project.pbxproj').write_text('// !$*UTF8*$!\n{archiveVersion=1;classes={};objectVersion=56;objects={\n'+'\n'.join(objects)+f'\n}};rootObject={project};}}\n')
print(f'Generated project: {len(sources)} translation units')
