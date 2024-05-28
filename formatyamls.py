import ruamel.yaml
import re
import sys
import getopt
import os

def convert_offset_to_hex(yaml_file):
    yaml = ruamel.yaml.YAML()
    with open(yaml_file, 'r') as f:
        data = yaml.load(f)
    if data is not None:
        for node_key, node_data in data.items():
            if 'offset' in node_data:
                node_data['offset'] = ruamel.yaml.scalarint.HexCapsInt(node_data['offset'])
            elif 'mio0' in node_data:
                node_data['mio0'] = ruamel.yaml.scalarint.HexCapsInt(node_data['mio0'])

        with open(yaml_file, 'w') as f:
            yaml.dump(data, f)

def sort_nodes(data):
    if data is None:
        return data
    res = {}
    initial_dict = {}
    for key in data.keys():
        if "offset" in data[key]:
            initial_dict[key] = data[key]["offset"]

    sorted_dict = dict(sorted(initial_dict.items(), key=lambda item: item[1]))

    for new_key in sorted_dict.keys():
        res[new_key] = data[new_key]

    return res

def sort(yaml_file):
    yaml = ruamel.yaml.YAML()
    with open(yaml_file, 'r') as f:
        data = yaml.load(f)

    if data is not None:
        with open(yaml_file, 'w') as f:
            yaml.dump(sort_nodes(data), f)

def fix_segments_format(yaml_file):
    with open (yaml_file, "r") as f:
        s = f.read()
    ret = re.sub(r"  - - ([0-9xA-F]+)\n.   - ([0-9xA-F]+)",r"    - [\1, \2]", s)
    with open(yaml_file, 'w') as f:
        f.write(ret)

def format(yaml_file):
    with open (yaml_file, "r") as f:
        s = f.read()
    ret = re.sub(r"([A-Za-z0-9]+)\n([\.A-Za-z0-9_-]+):",r"\1\n\n\2:", s)
    ret = re.sub(r"\n\n([\n])+",r"\n\n", ret)
    with open(yaml_file, 'w') as f:
        f.write(ret)

def grab_config(yaml_file):
    with open (yaml_file, "r") as f:
        s = f.read()
    matches = re.finditer(r":config:(\n  [^\n]+)+", s)

    # Return the matched sections
    output = ""
    for match in matches:
        output += match.group(0)  # Entire matched section

    ret = re.sub(r":config:(\n  [^\n]+)+",r"", s)
    with open(yaml_file, 'w') as f:
        f.write(ret)

    return output
    
def re_add_config(yaml_file, config):
    with open (yaml_file, "r") as f:
        s = f.read()
    ret = ""
    if config != "":
        ret += config + "\n\n"
    ret += s
    ret = re.sub(r"\n\n([\n])+",r"\n\n", ret)
    with open(yaml_file, 'w') as f:
        f.write(ret)

def process_file(yaml_file):
    config = grab_config(yaml_file)
    convert_offset_to_hex(yaml_file)
    sort(yaml_file)
    convert_offset_to_hex(yaml_file)
    fix_segments_format(yaml_file)
    format(yaml_file)
    re_add_config(yaml_file, config)

def main(argv):
    opts, args = getopt.getopt(argv,"hd:f:",["help","dir=","file="])
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            print("This is a tool for formatting Torch's asset yamls, and sorts the assets by offset.\n")
            print(" -d / --dir : root directory of asset yamls, which will be walked to find all yaml files to format.")
            print(" -f / --file : path to specific yaml file you want to format.")
            sys.exit()
        elif opt in ("-d", "--dir"):
            dir = arg
            for dirpath, subdirs, files in os.walk(dir):
                for x in files:
                    file = os.path.join(dirpath, x)
                    if ".yml" not in file and ".yaml" not in file:
                        continue
                    # Skip sound yamls and dialog strings
                    if "sound/" in file or "strings.yml" in file:
                        continue
                    process_file(file)
            sys.exit()
        elif opt in ("-f", "--file"):
            if ".yml" in arg or ".yaml" in arg:
                process_file(arg)
            else:
                print("Provided file is not a yaml file. Please check the extension matches '.yml' or '.yaml'")
            sys.exit()
    print("\nInvalid arguments. Use -h / --help to find out more.\n")
    sys.exit()

if __name__ == "__main__":
    main(sys.argv[1:])
