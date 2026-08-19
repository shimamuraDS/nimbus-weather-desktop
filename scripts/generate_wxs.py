#!/usr/bin/env python3
"""Legacy WiX v4 source generator.

The supported release pipeline is scripts/build_release.py, which uses CMake
Install, Qt's deployment API and CPack. This module remains only for inspecting
or reproducing packages created by the earlier hand-authored WiX workflow.
"""
import os, sys, uuid, argparse, hashlib
from pathlib import Path
from xml.sax.saxutils import escape

APP_DISPLAY_NAME = 'Nimbus Weather'
APP_VERSION = '1.0.2'
EXECUTABLE_NAME = 'NimbusWeather.exe'
ICON_FILENAME = 'NimbusWeather.ico'

INVALID_CHARS = set('- .+~!@#$%^&*()=[]{}|;:\'",<>/?`')
VALID_REPLACEMENTS = {
    '-': '_', ' ': '_', '.': '_', '+': '_', '~': '_', '!': '_',
    '@': '_', '#': '_', '$': '_', '%': '_', '^': '_', '&': '_',
    '*': '_', '(': '_', ')': '_', '=': '_', '[': '_', ']': '_',
    '{': '_', '}': '_', '|': '_', ';': '_', ':': '_', "'": '_',
    '"': '_', ',': '_', '<': '_', '>': '_', '/': '_', '?': '_',
    '`': '_',
}

def id_from_path(path: str) -> str:
    """Create a valid WiX Id from a full relative path."""
    result = []
    for ch in path:
        if ch in VALID_REPLACEMENTS:
            result.append(VALID_REPLACEMENTS[ch])
        elif ch.isalnum():
            result.append(ch)
        else:
            result.append('_')
    return ''.join(result)

def build_dir_tree(root: Path):
    """Build a nested directory structure from all files under root."""
    tree = {}
    for f in root.rglob('*'):
        if f.is_file() and f.suffix != '.wxs' and f.suffix != '.wixpdb':
            parts = f.relative_to(root).parts
            d = tree
            for part in parts[:-1]:
                d = d.setdefault(part, {})
            d.setdefault('__FILES__', []).append(f)
    return tree

def collect_all_dirs(tree: dict, prefix_parts: tuple, result: list):
    """Collect all directory paths with their full prefix parts."""
    for name, children in sorted(tree.items()):
        if name == '__FILES__':
            continue
        full_parts = prefix_parts + (name,)
        result.append(full_parts)
        collect_all_dirs(children, full_parts, result)

def write_dir_xml(fh, tree: dict, prefix_parts: tuple, indent: int):
    """Recursively write Directory elements with unique IDs."""
    pfx = ' ' * indent

    for name, children in sorted(tree.items()):
        if name == '__FILES__':
            continue
        full_parts = prefix_parts + (name,)
        dir_id = f"DIR_{id_from_path('/'.join(full_parts))}"
        fh.write(f'{pfx}<Directory Id="{dir_id}" Name="{escape(name)}">\n')
        write_dir_xml(fh, children, full_parts, indent + 2)
        fh.write(f'{pfx}</Directory>\n')

def write_components(fh, tree: dict, prefix_parts: tuple, indent: int):
    """Write Component elements with unique File Ids."""
    pfx = ' ' * indent
    counter = [0]

    def walk(t, path_parts):
        for name, children in sorted(t.items()):
            if name == '__FILES__':
                for fpath in children:
                    counter[0] += 1
                    rel_path = fpath.relative_to(Path.cwd()).as_posix()
                    comp_id = f"cmp{counter[0]:05d}"
                    # Generate unique File ID from relative path
                    file_id = "fil_" + id_from_path(rel_path)
                    sp = str(fpath).replace('\\', '\\\\')
                    dir_part = '/'.join(path_parts)
                    if dir_part:
                        dir_id = f"DIR_{id_from_path(dir_part)}"
                        fh.write(f'{pfx}<Component Id="{comp_id}" Guid="*" Directory="{dir_id}">\n')
                    else:
                        fh.write(f'{pfx}<Component Id="{comp_id}" Guid="*">\n')
                    fh.write(f'{pfx}  <File Id="{file_id}" Source="{sp}" KeyPath="yes" />\n')
                    fh.write(f'{pfx}</Component>\n')
            else:
                new_parts = path_parts + (name,)
                walk(children, new_parts)

    walk(tree, prefix_parts)
    return counter[0]

def generate_wxs(deploy_dir: Path, output: Path, product_name: str,
                 version: str, upgrade_code: str):
    deploy_dir = deploy_dir.resolve()
    output = output.resolve()
    executable_path = deploy_dir / EXECUTABLE_NAME
    icon_path = Path(__file__).resolve().parent.parent / 'resources' / ICON_FILENAME
    if not executable_path.is_file():
        raise FileNotFoundError(f'Missing application executable: {executable_path}')
    if not icon_path.is_file():
        raise FileNotFoundError(f'Missing application icon: {icon_path}')

    tree = build_dir_tree(deploy_dir)

    os.chdir(deploy_dir)

    with open(output, 'w', encoding='utf-8') as f:
        f.write('<?xml version="1.0" encoding="utf-8"?>\n')
        f.write('<Wix xmlns="http://wixtoolset.org/schemas/v4/wxs"\n')
        f.write('     xmlns:ui="http://wixtoolset.org/schemas/v4/wxs/ui">\n')
        f.write(f'  <Package Name="{escape(product_name)}" Manufacturer="shimamuraDS"\n')
        f.write(f'           Version="{escape(version)}" UpgradeCode="{upgrade_code}"\n')
        f.write( '           Scope="perMachine" Language="2052" Codepage="936">\n\n')
        f.write( '    <MajorUpgrade DowngradeErrorMessage="A newer version is already installed." />\n\n')
        f.write( '    <MediaTemplate EmbedCab="yes" />\n\n')
        f.write( '    <ui:WixUI Id="WixUI_InstallDir" />\n')
        f.write( '    <Property Id="WIXUI_INSTALLDIR" Value="INSTALLFOLDER" />\n\n')
        ico_path = str(icon_path).replace('\\', '\\\\')
        f.write(f'    <Icon Id="NimbusIcon" SourceFile="{ico_path}" />\n')
        f.write( '    <Property Id="ARPPRODUCTICON" Value="NimbusIcon" />\n')
        f.write( '    <Property Id="WIXUI_EXITDIALOGOPTIONALCHECKBOXTEXT" Value="&#x542F;&#x52A8; Nimbus Weather" />\n')
        f.write( '    <Property Id="WixShellExecTarget" Value="[INSTALLFOLDER]NimbusWeather.exe" />\n\n')

        # Directory tree
        f.write('    <StandardDirectory Id="ProgramFiles64Folder">\n')
        f.write('      <Directory Id="INSTALLFOLDER" Name="Nimbus Weather">\n')

        write_dir_xml(f, tree, (), 8)

        f.write('      </Directory>\n')
        f.write('    </StandardDirectory>\n\n')

        # Start Menu - use a non-standard ID
        f.write('    <StandardDirectory Id="ProgramMenuFolder">\n')
        f.write('      <Directory Id="NimbusWeatherStartMenuFolder" Name="Nimbus Weather" />\n')
        f.write('    </StandardDirectory>\n\n')

        # Desktop
        f.write('    <StandardDirectory Id="DesktopFolder" />\n\n')

        # Main component group
        f.write('    <ComponentGroup Id="AppComponents" Directory="INSTALLFOLDER">\n')
        count = write_components(f, tree, (), 6)
        f.write('    </ComponentGroup>\n\n')

        # Shortcuts
        f.write('    <ComponentGroup Id="ShortcutComponents" Directory="NimbusWeatherStartMenuFolder">\n')
        f.write('      <Component Id="StartMenuShortcut" Guid="*">\n')
        f.write('        <Shortcut Id="StartMenuSC" Name="Nimbus Weather"\n')
        f.write('                  Target="[INSTALLFOLDER]NimbusWeather.exe"\n')
        f.write('                  Icon="NimbusIcon"\n')
        f.write('                  WorkingDirectory="INSTALLFOLDER" />\n')
        f.write('        <RegistryValue Root="HKCU" Key="Software\\NimbusWeather"\n')
        f.write('                       Name="installed" Type="integer" Value="1" KeyPath="yes" />\n')
        f.write('      </Component>\n')
        f.write('    </ComponentGroup>\n\n')

        f.write('    <ComponentGroup Id="DesktopShortcutComponents" Directory="DesktopFolder">\n')
        f.write('      <Component Id="DesktopShortcut" Guid="*">\n')
        f.write('        <Shortcut Id="DesktopSC" Name="Nimbus Weather"\n')
        f.write('                  Target="[INSTALLFOLDER]NimbusWeather.exe"\n')
        f.write('                  Icon="NimbusIcon"\n')
        f.write('                  WorkingDirectory="INSTALLFOLDER" />\n')
        f.write('        <RegistryValue Root="HKCU" Key="Software\\NimbusWeather"\n')
        f.write('                       Name="desktopInstalled" Type="integer" Value="1" KeyPath="yes" />\n')
        f.write('      </Component>\n')
        f.write('    </ComponentGroup>\n\n')

        # Uninstall shortcut in install directory
        f.write('    <Component Id="UninstallShortcut" Guid="*" Directory="INSTALLFOLDER">\n')
        f.write('      <Shortcut Id="UninstallSC" Name="Uninstall Nimbus Weather"\n')
        f.write('                Target="[SystemFolder]msiexec.exe"\n')
        f.write('                Arguments="/x [ProductCode]"\n')
        f.write('                Description="Uninstall Nimbus Weather"\n')
        f.write('                WorkingDirectory="INSTALLFOLDER" />\n')
        f.write('      <RegistryValue Root="HKCU" Key="Software\\NimbusWeather"\n')
        f.write('                     Name="uninstallShortcut" Type="integer" Value="1" KeyPath="yes" />\n')
        f.write('    </Component>\n\n')

        # Auto-start registry
        f.write('    <Component Id="AutostartRegistry" Guid="*" Directory="INSTALLFOLDER">\n')
        f.write('      <RegistryValue Root="HKCU"\n')
        f.write('                     Key="Software\\Microsoft\\Windows\\CurrentVersion\\Run"\n')
        f.write('                     Name="Nimbus Weather"\n')
        f.write('                     Value="&quot;[INSTALLFOLDER]NimbusWeather.exe&quot; -hidden"\n')
        f.write('                     Type="string" KeyPath="yes" />\n')
        f.write('    </Component>\n\n')

        # Feature
        f.write('    <Feature Id="Main" Title="Nimbus Weather" Level="1">\n')
        f.write('      <ComponentGroupRef Id="AppComponents" />\n')
        f.write('      <ComponentGroupRef Id="ShortcutComponents" />\n')
        f.write('      <ComponentGroupRef Id="DesktopShortcutComponents" />\n')
        f.write('      <ComponentRef Id="UninstallShortcut" />\n')
        f.write('      <ComponentRef Id="AutostartRegistry" />\n')
        f.write('    </Feature>\n\n')

        f.write('  </Package>\n')
        f.write('</Wix>\n')

    print(f"Generated {output} with {count} file components")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('deploy_dir', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--name', default=APP_DISPLAY_NAME)
    parser.add_argument('--version', default=APP_VERSION)
    parser.add_argument('--upgrade-code', required=True)
    args = parser.parse_args()
    generate_wxs(args.deploy_dir, args.output, args.name,
                 args.version, args.upgrade_code)
