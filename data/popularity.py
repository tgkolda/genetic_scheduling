import yaml
from scholarly import scholarly # Google scholar API

# Scraped yaml file
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/minisymposia.yaml') as minifile:  
    mini_list = yaml.load(minifile, Loader=yaml.FullLoader)

with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/citations.yaml') as file:
    citations = yaml.load(file, Loader=yaml.FullLoader)

for key, value in mini_list.items():
    nspeakers = len(value['speakers'])
    for speaker in value['speakers']:
        if speaker in citations:
            continue
        print(speaker, ' not found')
        continue
        search_query = scholarly.search_author(speaker)
        try:
            result = next(search_query)
            author = scholarly.fill(result)
            if 'citedby' in author:
                citations[speaker] = author['citedby']
        except StopIteration:
            print('Could not find author ' + speaker)
    
# Saves the new yaml
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/citations.yaml', 'w') as file:
    yaml.dump(citations, file)
