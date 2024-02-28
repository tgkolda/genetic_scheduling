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

        # This score will be high for simple misspellings
        # For example,
        # Sri Hari Krishna Narayanan and
        # Sri Hari Krishn Narayanan
        # score a 98% similarity
        # Bart Van Bloemen Waanders and
        # Bart van Bloeme Waanders
        # score a 94% similarity
        ratio = fuzz.ratio(name1, name2)

        # This score accounts for middle names/initials
        # For example,
        # Patrick Farrell and
        # Patrick E. Farrell
        # score a 100% similarity
        tsratio = fuzz.token_set_ratio(name1, name2)

        best_ratio = max(ratio, tsratio)
        if best_ratio > 80:
            name_pairs.append(f'{name1}\n{name2}')
            scores.append(best_ratio)

npscores = np.array(scores)
perm = np.argsort(npscores)
perm = np.flip(perm)

for i in perm:
    print(f'{name_pairs[i]}\n{scores[i]}\n')