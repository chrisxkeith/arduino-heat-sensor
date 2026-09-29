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
    output_file = 'data.txt'
    nRows = 0
    with open(output_file, mode='w', newline='') as outfile:
        with open(input_file, mode='r', newline='') as file:
            reader = csv.DictReader(file)
            for row in reader:
                outfile.write('"' + row['time'] + '","' + row['value'] + '"\n')
                nRows += 1
    print("Total rows processed: " + str(nRows))

if __name__ == "__main__":
    main()
