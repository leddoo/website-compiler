import sys

def make_array(binary, name):
    result = "U8 " + name + "[] = {\n"
    line = "   "

    for byte in binary:
        string = str(byte)

        if len(line) + 1 + len(string) < 80:
            line += " "
        else:
            line += "\n"
            result += line
            line = "    "

        line += string
        line += ","

    result += line
    result += "\n};\n"
    result += "Usize " + name + "_size = sizeof(" + name + ");\n"
    return result


def make_array_from_file(path, name):
    f = open(path, "rb")
    contents = f.read()
    f.close()

    result = make_array(contents, name)
    return result


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: \"{} (name, path)+ output\".".format(sys.argv[0]))
        exit(0)

    result = ""
    i = 1
    while i + 1 < len(sys.argv):
        name = sys.argv[i + 0]
        path = sys.argv[i + 1]
        i += 2

        result += make_array_from_file(path, name)

    if i >= len(sys.argv):
        print("Error: No output given.")
        exit(1)

    f = open(sys.argv[-1], "w")
    f.write(result)
    f.close()

