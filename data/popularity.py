import csv
import yaml
from scholarly import scholarly # Google scholar API

speakers = []
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/participants.csv',encoding='utf8') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    for row in csv_reader:
        if line_count == 0:
            line_count += 1
        else:
            # Only record the speakers
            name = row[0]
            role = row[1]
            affiliation = row[3].split(',')[0]
            
            if role == "mini speaker":
                speakers.append([name, affiliation])
            line_count += 1
#print(speakers)

with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/new_citations.yaml') as file:
    dict = yaml.load(file, Loader=yaml.FullLoader)

for name, affiliation in speakers:
    break
    if name in dict:
        continue
    search_query = scholarly.search_author(name + ' ' + affiliation)
    try:
        result = next(search_query)
        author = scholarly.fill(result, sections=['publications'], sortby='year')
        total_citations = 0
        for publication in author['publications']:
            if 'pub_year' in publication['bib']:
                year = publication['bib']['pub_year']
            else:
                continue
            if int(year) < 2017:
                break
            citations = publication['num_citations']
            total_citations += citations
        dict[name] = total_citations
        print(name + ' ' + affiliation, 'has', total_citations)
        continue
    except StopIteration:
        print('Could not find', name + ' ' + affiliation)

    search_query = scholarly.search_author(name)
    try:
        result = next(search_query)
        author = scholarly.fill(result, sections=['publications'], sortby='year')
        total_citations = 0
        for publication in author['publications']:
            if 'pub_year' in publication['bib']:
                year = publication['bib']['pub_year']
            else:
                continue
            if int(year) < 2017:
                break
            citations = publication['num_citations']
            total_citations += citations
        dict[name] = total_citations
        print(name, 'has', total_citations)
    except StopIteration:
        print('Could not find', name)

# Reverse the first and last name
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/minisymposia.yaml') as file:
    mini = yaml.load(file, Loader=yaml.FullLoader)

unscrambled_dict = {}
for name in dict:
    words = name.split()
    num_words = len(words)
    found = False
    for first_name_index in range(1,num_words):
        first_name = ' '.join(words[first_name_index:])
        last_name = ' '.join(words[:first_name_index])
        search_name = first_name + ' ' + last_name

        for title in mini:
            for speaker in mini[title]['speakers']:
                if speaker == search_name:
                    unscrambled_dict[search_name] = dict[name]
                    found = True
                    break
            if found:
                break
        if found:
            break
    if not found:
        print('Could not find', name)

# Saves the new yaml
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/newest_citations.yaml', 'w') as file:
    yaml.dump(unscrambled_dict, file)
