# Based on https://towardsdatascience.com/evolution-of-a-salesman-a-complete-genetic-algorithm-tutorial-for-python-6fe5d2b3ca35

import numpy as np
import yaml

nslots = 11
npop = 5

with open(r'data/rooms.yaml') as roomfile:
    room_list = yaml.load(roomfile, Loader=yaml.FullLoader)
    rooms = list(room_list.keys())
    print(rooms)

    print(room_list)
    nrooms = len(room_list)
    
    with open(r'data/minisymposia.yaml') as minifile:
        mini_list = yaml.load(minifile, Loader=yaml.FullLoader)

        print(mini_list)
        titles = list(mini_list.keys())
        nmini = len(mini_list)

        # Make an initial mapping of minisymposia to rooms/slots
        ngenes = nrooms*nslots
        mapping = np.zeros(ngenes, dtype=np.int32)
        for i in range(nmini):
            mapping[i] = i+1

        # Make an initial population by permuting that mapping
        population = np.zeros((ngenes, npop), dtype=np.int32)
        
        rng = np.random.default_rng()
        for i in range(npop):
            population[:,i] = rng.permutation(mapping)

        def print_schedule(schedule):
            s = schedule.reshape(nslots, nrooms)
            for r in range(nslots):
                print(f'Slot {r+1}')
                for c in range(nrooms):
                    tid = s[r,c] - 1
                    room = rooms[c]

                    if tid >= 0:
                        print(f'{titles[tid]}: {room}')

        for i in range(npop):
            print(f'Schedule {i+1}:')
            print_schedule(population[:,i])