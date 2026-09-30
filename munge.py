import csv
import os
import platform

def main():
    if platform.system() != 'Linux':
        print("This script should be run on a Linux system.")
        return
    fn = 'StovetopTemp_prod-gridAsString.csv'
    dir = '/home/ck/Downloads/'
    for root, dirs, files in os.walk(dir):
        for file in files:
            if file.endswith(fn):
                input_file = os.path.join(root, file)
                break
    if not input_file:
        print(fn + " file not found in " + dir)
        return
    print("Input file: " + input_file)
    output_file = 'data.txt'
    nInputRows = 0
    nOutputRows = 0
    with open(output_file, mode='w', newline='') as outfile:
        with open(input_file, mode='r', newline='') as file:
            reader = csv.DictReader(file)
            for row in reader:
                if not row['value'].startswith(" -"):
                    outfile.write('"' + row['time'] + '","' + row['value'] + '"\n')
                    nOutputRows += 1
                nInputRows += 1
    print("Total input rows processed: " + str(nInputRows))
    print("Total output rows processed: " + str(nOutputRows))

if __name__ == "__main__":
    main()
