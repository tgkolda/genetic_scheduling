import csv
import yaml

participants = []
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/participants.csv',encoding='utf8') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    for row in csv_reader:
        if line_count == 0:
            line_count += 1
        else:
            # Only record the organizers
            name = row[0]
            role = row[1]
            
            if role == "ORG: Mini organizer":
                participants.append(name)
            line_count += 1
    participants = set(participants)

minisymposia = {}
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/minisymposia.csv',encoding='utf8') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    for row in csv_reader:
        if line_count == 0:
            line_count += 1
        else:
            codes = row[0:3]
            session_no = row[3]
            title = row[4]
            organizers = row[5].split(',')
            sessions = row[6:11]

            speakers = []
            talks = []
            for session in sessions:
                if session == '' or session == 'NONE':
                    continue
                if session == 'TBD':
                    speakers.append('TBD')
                    talks.append('TBD')
                    continue
                words = session.split(' - ')
                speakers.append(words[0])
                talks.append(words[2])

            minisymposia[title] = {}
            minisymposia[title]["session number"] = int(session_no)
            minisymposia[title]["organizers"] = []
            for organizer in organizers:
                organizer = organizer.strip()
                numhits = 0
                for participant in participants:
                    if participant.startswith(organizer + " "):
                        strlen = len(organizer)
                        solution = participant[strlen+1:] + " " + organizer
                        numhits+=1
                if numhits != 1:
                    print("There are too many hits for", organizer)
                    minisymposia[title]["organizers"].append(organizer)
                else:
                    minisymposia[title]["organizers"].append(solution)     

            minisymposia[title]["speakers"] = speakers
            minisymposia[title]["talks"] = talks
            minisymposia[title]["class codes"] = list(map(int, codes))
            line_count += 1

with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/minisymposia.yaml', 'w') as file:
    yaml.dump(minisymposia, file)

lectures = {}
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/lectures.csv',encoding='utf8') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    for row in csv_reader:
        if line_count == 0:
            line_count += 1
        else:
            title = row[8]
            speaker = row[6] + ' ' + row[5]
            codes = row[1:4]
            id = row[4]

            lectures[title] = {}
            lectures[title]["speaker"] = speaker
            lectures[title]["id"] = int(id)
            lectures[title]["class codes"] = list(map(int, codes))
            line_count += 1

with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/lectures.yaml', 'w') as file:
    yaml.dump(lectures, file)