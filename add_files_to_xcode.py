#!/usr/bin/env python3
"""
Script to add new Atari800 files to the Xcode project.
"""

import re
import sys

# File list with source and header files
files_to_add = [
    ('netsio.c', 'netsio.h'),
    ('artifact.c', 'artifact.h'),
    ('filter_ntsc.c', 'filter_ntsc.h'),
    ('pal_blending.c', 'pal_blending.h'),
    ('colours_ntsc.c', 'colours_ntsc.h'),
    ('colours_pal.c', 'colours_pal.h'),
    ('colours_external.c', 'colours_external.h'),
    ('videomode.c', 'videomode.h'),
    ('pokeyrec.c', 'pokeyrec.h'),
    ('voicebox.c', 'voicebox.h'),
    ('votraxsnd.c', 'votraxsnd.h'),
    ('atari_ntsc/atari_ntsc.c', 'atari_ntsc/atari_ntsc.h'),
]

# Generate unique IDs for file references and build files
def generate_id_pairs():
    """Generate unique hex IDs for file references and build files."""
    ids = []
    base = 0x2DF00000  # Starting from a high base to avoid conflicts
    
    for i, (src_file, hdr_file) in enumerate(files_to_add):
        src_file_ref = f"{base + i*4:08X}25CF99D0{base>>16:04X}DE9C"
        src_build_file = f"{base + i*4 + 1:08X}25CF99D0{base>>16:04X}DE9C"
        hdr_file_ref = f"{base + i*4 + 2:08X}25CF99D0{base>>16:04X}DE9C"
        hdr_build_file = f"{base + i*4 + 3:08X}25CF99D0{base>>16:04X}DE9C"
        
        ids.append({
            'src_file': src_file,
            'hdr_file': hdr_file,
            'src_file_ref': src_file_ref,
            'src_build_file': src_build_file,
            'hdr_file_ref': hdr_file_ref,
            'hdr_build_file': hdr_build_file
        })
    
    return ids

def main():
    project_file = 'atari800-MacOSX/src/Atari800MacX/Atari800MacX.xcodeproj/project.pbxproj'
    
    with open(project_file, 'r') as f:
        content = f.read()
    
    ids = generate_id_pairs()
    
    # Add PBXBuildFile entries
    build_file_section = ""
    for entry in ids:
        build_file_section += f"\t\t{entry['src_build_file']} /* {entry['src_file']} in Sources */ = {{isa = PBXBuildFile; fileRef = {entry['src_file_ref']} /* {entry['src_file']} */; }};\n"
        build_file_section += f"\t\t{entry['hdr_build_file']} /* {entry['hdr_file']} in Headers */ = {{isa = PBXBuildFile; fileRef = {entry['hdr_file_ref']} /* {entry['hdr_file']} */; }};\n"
    
    # Insert before "End PBXBuildFile section"
    content = content.replace(
        "/* End PBXBuildFile section */",
        build_file_section + "/* End PBXBuildFile section */"
    )
    
    # Add PBXFileReference entries
    file_ref_section = ""
    for entry in ids:
        src_path = f"../{entry['src_file']}"  # Files are in ../src relative to Xcode project
        hdr_path = f"../{entry['hdr_file']}"
        file_ref_section += f"\t\t{entry['src_file_ref']} /* {entry['src_file']} */ = {{isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c.c; path = \"{src_path}\"; sourceTree = \"<group>\"; }};\n"
        file_ref_section += f"\t\t{entry['hdr_file_ref']} /* {entry['hdr_file']} */ = {{isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.c.h; path = \"{hdr_path}\"; sourceTree = \"<group>\"; }};\n"
    
    # Insert before "End PBXFileReference section"
    content = content.replace(
        "/* End PBXFileReference section */",
        file_ref_section + "/* End PBXFileReference section */"
    )
    
    # Add to Headers build phase
    headers_entries = ""
    for entry in ids:
        headers_entries += f"\t\t\t\t{entry['hdr_build_file']} /* {entry['hdr_file']} in Headers */,\n"
    
    # Find headers build phase and add entries before the closing parenthesis
    pattern = r"(2DAEDB0009B69AED005FF181 /\* monitor\.h in Headers \*/,\n)"
    content = re.sub(pattern, r"\1" + headers_entries, content)
    
    # Add to Sources build phase
    sources_entries = ""
    for entry in ids:
        sources_entries += f"\t\t\t\t{entry['src_build_file']} /* {entry['src_file']} in Sources */,\n"
    
    # Find sources build phase and add entries before a known entry
    pattern = r"(2DAEDB7009B69AED005FF181 /\* SDLMain\.m in Sources \*/,\n)"
    content = re.sub(pattern, r"\1" + sources_entries, content)
    
    # Write modified content back
    with open(project_file, 'w') as f:
        f.write(content)
    
    print("Successfully added all files to Xcode project!")
    print(f"Added {len(files_to_add)} source/header file pairs")
    
    # Print the file IDs for reference
    print("\nGenerated file reference IDs:")
    for entry in ids:
        print(f"{entry['src_file']}: {entry['src_file_ref']}")
        print(f"{entry['hdr_file']}: {entry['hdr_file_ref']}")

if __name__ == '__main__':
    main()