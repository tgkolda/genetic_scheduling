from thefuzz import fuzz
import yaml
import numpy as np

# Read the minisymposia
with open(r'data/SIAM-CSE23/minisymposia.yaml') as file:
    dict = yaml.load(file, Loader=yaml.FullLoader)

# Get the list of names
names = []
for key, value in dict.items():
    if 'organizers' in value:
        for name in value['organizers']:
            names.append(name)
    if 'speakers' in value:
        for name in value['speakers']:
            names.append(name)

# Remove duplicates
names = list(set(names))

# Compute the similarities
name_pairs = []
scores = []
n = len(names)
for i in range(n):
    name1 = names[i]
    for j in range(i+1, n):
        name2 = names[j]
        ratio = fuzz.ratio(name1, name2)
        if ratio > 80:
            name_pairs.append(f'{name1}\n{name2}')
            scores.append(ratio)

npscores = np.array(scores)
perm = np.argsort(npscores)
perm = np.flip(perm)

for i in perm:
    print(f'{name_pairs[i]}\n{scores[i]}\n')