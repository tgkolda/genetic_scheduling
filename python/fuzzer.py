from thefuzz import fuzz
from nicknames import NickNamer
import yaml
import numpy as np

# Using nicknames is meant to match names like
# Chris Vogl and
# Christopher Vogl
# with a 100% similarity score.
# Unfortunately, this predominantly captures American nicknames,
# but there's no reason we can't improve the nickname database.
nn = NickNamer()

def get_nicknames(name):
    # Grab the first name to determine nicknames
    separated = name.split()
    first_name = separated[0]

    # Get a list of nicknames/canonical names
    first_names = nn.nicknames_of(separated[0]) | nn.canonicals_of(separated[0])
    first_names.add(first_name)

    # Add back the surnames
    full_names = []
    for first_name in first_names:
        full_name = ' '.join([first_name] + separated[1:])
        full_names.append(full_name)
    return full_names

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
    names1 = get_nicknames(names[i])
    # Compare against other names
    for j in range(i+1, n):
        names2 = get_nicknames(names[j])
        best_ratio = 0
        for name1 in names1:
            for name2 in names2:
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

                best_ratio = max(ratio, tsratio, best_ratio)

        if best_ratio > 80:
            name_pairs.append(f'{names[i]}\n{names[j]}')
            scores.append(best_ratio)

npscores = np.array(scores)
perm = np.argsort(npscores)
perm = np.flip(perm)

for i in perm:
    print(f'{name_pairs[i]}\n{scores[i]}\n')