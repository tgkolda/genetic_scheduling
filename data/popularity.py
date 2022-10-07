import yaml
from scholarly import scholarly # Google scholar API

# Scraped yaml file
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/minisymposia_predicted_theme.yaml') as minifile:  
    mini_list = yaml.load(minifile, Loader=yaml.FullLoader)

for key, value in mini_list.items():
    pop = 0
    nspeakers = len(value['speakers'])
    for speaker in value['speakers']:
        search_query = scholarly.search_author(speaker)
        try:
            result = next(search_query)
            author = scholarly.fill(result)
            if 'citedby' in author:
                pop += author['citedby']
                print(speaker + ' has ' + str(author['citedby']) + ' citations')
        except StopIteration:
            print('Could not find author ' + speaker)
    if nspeakers > 0:
        mini_list[key]['estimated popularity'] = pop / nspeakers
    
# Saves the new yaml
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/minisymposia_predicted_popularity.yaml', 'w') as file:
    yaml.dump(mini_list, file)
