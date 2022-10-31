import csv
import yaml

minisymposia = {}
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/minisymposia.csv',encoding='utf8') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    for row in csv_reader:
        if line_count == 0:
            print(f'Column names are {", ".join(row)}')
            line_count += 1
        else:
            codes = row[0:3]
            title = row[4]
            organizers = row[5]
            sessions = row[6:11]

            speakers = []
            talks = []
            for session in sessions:
                if session == '':
                    continue
                words = session.split(' - ')
                speakers.append(words[0])
                talks.append(words[2])

            minisymposia[title] = {}
            minisymposia[title]["organizer"] = organizers
            minisymposia[title]["speakers"] = speakers
            minisymposia[title]["talks"] = talks
            minisymposia[title]["class codes"] = codes
            line_count += 1
    print(f'Processed {line_count} lines.')

with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/minisymposia.yaml', 'w') as file:
    yaml.dump(minisymposia, file)

lectures = {}
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/lectures.csv',encoding='utf8') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    for row in csv_reader:
        if line_count == 0:
            print(f'Column names are {", ".join(row)}')
            line_count += 1
        else:
            title = row[8]
            speaker = row[6] + ' ' + row[5]
            codes = row[1:4]

            lectures[title] = {}
            lectures[title]["speaker"] = speaker
            lectures[title]["class codes"] = codes
            line_count += 1
    print(f'Processed {line_count} lines.')

with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/lectures.yaml', 'w') as file:
    yaml.dump(lectures, file)