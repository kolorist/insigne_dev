import argparse
import json

class printHelp(argparse.Action):
    def __init__(self, option_strings, dest, **kwargs):
        super(printHelp, self).__init__(option_strings, dest, **kwargs)
    def __call__(self, parser, namespace, values, option_strings = None):
        print("TODO")

class testJSON(argparse.Action):
    def __init__(self, option_strings, dest, **kwargs):
        super(testJSON, self).__init__(option_strings, dest, **kwargs)
    def __call__(self, parser, namespace, values, option_strings = None):
        gltfFilePath = values[0]
        gltfJsonFile = open(gltfFilePath, 'r')
        gltfData = json.load(gltfJsonFile)

        textures = gltfData['textures']
        images = gltfData['images']
        materials = gltfData['materials']
        for mat in materials:
            if 'normalTexture' in mat:
                normalTex = mat['normalTexture']

                if 'pbrMetallicRoughness' in mat:
                    pbr = mat['pbrMetallicRoughness']
                    colorTex = pbr['baseColorTexture']
                    attribTex = pbr['metallicRoughnessTexture']

                    colorTexPath = images[textures[colorTex['index']]['source']]['uri']
                    normalTexPath = images[textures[normalTex['index']]['source']]['uri']
                    attribTexPath = images[textures[attribTex['index']]['source']]['uri']

                    print(colorTexPath + ' ' + normalTexPath + ' ' + attribTexPath)

def main():
    parser = argparse.ArgumentParser(description = "GLTF scene baker")
    parser.add_argument('--howto', action = printHelp, nargs = 0)
    parser.add_argument('--json', action = testJSON, nargs = 1)
    args = parser.parse_args()

if __name__ == "__main__":
    main()
