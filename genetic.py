# Based on https://towardsdatascience.com/evolution-of-a-salesman-a-complete-genetic-algorithm-tutorial-for-python-6fe5d2b3ca35

import matplotlib.pyplot as plt 
import numpy as np
import operator
import pandas as pd
import random
import re
import yaml

from itertools import combinations

class Minisymposium:
    def __init__(self, title, participants, part, parts):
        self.title = title
        self.participants = set(participants)
        self.part = part
        self.parts = parts

    def overlaps_participants(self, mini):
        return self.participants & mini.participants

    def goes_before(self, mini):
        if self.title == mini.title:
            return self.part < mini.part
        return False

class Minisymposia:
    def __init__(self, mini_list):
        self.mini_list = mini_list

        # Determine the speaker conflicts
        nmini = len(mini_list)
        self.conflicts = np.zeros((nmini, nmini), dtype=np.bool8)
        self.ordering = [ [] for _ in range(nmini) ]

        # Determine the speaker conflicts and talks with a necessary ordering
        comb = combinations(range(nmini), 2)
        for m1, m2 in list(comb):
            self.conflicts[m1, m2] = mini_list[m1].overlaps_participants(mini_list[m2])
            self.conflicts[m2, m1] = self.conflicts[m1, m2]

            if mini_list[m1].goes_before(mini_list[m2]):
                self.ordering[m1].append(m2)
            elif mini_list[m2].goes_before(mini_list[m1]):
                self.ordering[m2].append(m1)

        self.nconflicts = np.count_nonzero(self.conflicts)/2
        self.nprereqs = sum( [ len(listElem) for listElem in self.ordering])

    def __len__(self):
        return len(self.mini_list)

class Room:
    def __init__(self, name, capacity):
        self.name = name
        self.capacity = capacity

class Schedule:
    rooms = []
    nslots = 15
    rng = np.random.default_rng()

    def __init__(self, eventList, permute=False):
        if permute:
            self.events = Schedule.rng.permutation(eventList)
        else:
            self.events = eventList

    def __len__(self):
        return len(self.events)

    def __getitem__(self, item):
         return self.events[item]

    def __setitem__(self, key, value):
        self.events[key] = value

    # if this schedule needs more rooms, return the number of extra rooms needed
    def needs_more_rooms(self):
        nextra = 0
        rid = 0 # room index
        for s in self.events:
            if s <= 0:
                rid = 0
            else:
                if rid >= len(Schedule.rooms):
                    nextra += 1
                rid = rid + 1
        return nextra

    def oversubscribes_participant(self):
        noversubscribed = 0
        nevents = len(self.events)
        for i in range(nevents):
            id1 = self.events[i]
            if  id1 > 0:
                for j in range(i+1, nevents):
                    id2 = self.events[j]
                    if id2 > 0:
                        if(minisymposia.conflicts[id1-1, id2-1]):
                            noversubscribed += 1
                    else:
                        # We found a delimiter
                        break
        return noversubscribed

    def comes_before(self, first, second):
        pos1 = np.where(self.events == first+1)[0][0]
        pos2 = np.where(self.events == second+1)[0][0]

        # Make sure there's at least one delimiter between them
        for i in range(pos1, pos2):
            if self.events[i] <= 0:
                return True

        return False

    def violates_order(self):
        violations = 0
        for i in range(len(minisymposia)):
            for j in minisymposia.ordering[i]:
                if not self.comes_before(i, j):
                    violations += 1
        return violations

    def is_possible(self):
        return self.needs_more_rooms() == 0 and self.oversubscribes_participant() == 0 and self.violates_order() == 0

    def __repr__(self):
        if not self.is_possible():
            return 'impossible'

        slot = 1
        rid = 0 # room index
        str = f'Slot {slot}\n'
        for s in self.events:
            if s <= 0:
                slot = slot + 1
                str += f'Slot {slot}\n'
                rid = 0
            else:
                if rid >= len(Schedule.rooms):
                    return 'impossible'
                str += f'{minisymposia[s-1].title}: {Schedule.rooms[rid].name}\n'
                rid = rid + 1
        return str

class Fitness:
    def __init__(self, schedule):
        self.schedule = schedule
        self.rating = np.nan

    # fitness is in the range [0, 1] with 1 being best
    def schedFitness(self):
        if np.isnan(self.rating):
            nmini = len(minisymposia)
            nrooms = len(Schedule.rooms)
            # Maximum penalty for needing more rooms is nmini - nrooms
            # Maximum penalty for oversubscribing participants is the total number of conflicts
            # Maximum penalty for order violations is the total number of prerequisites
            max_penalty = (nmini - nrooms) + minisymposia.nconflicts + minisymposia.nprereqs
            self.rating = 1 - (self.schedule.needs_more_rooms() + self.schedule.oversubscribes_participant() + self.schedule.violates_order()) / max_penalty
        return self.rating

def makeEventList():
    nmini = len(minisymposia)
    ngenes = nmini + Schedule.nslots - 1 
    eventList = np.zeros(ngenes, dtype=np.int32)
    for i in range(nmini):
        eventList[i] = i+1
    for i in range(nmini, ngenes):
        eventList[i] = nmini - (i + 1)
    return eventList

def initialPopulation(popSize):
    eventList = makeEventList()
    pop = []
    for i in range(popSize):
        pop.append(Schedule(eventList, True))
    return pop

def rankSchedules(population):
    fitnessResults = {}
    for i in range(0,len(population)):
        fitnessResults[i] = Fitness(population[i]).schedFitness()
    return sorted(fitnessResults.items(), key = operator.itemgetter(1), reverse=True)

def selection(popRanked, eliteSize):
    selectionResults = []
    df = pd.DataFrame(np.array(popRanked), columns=["Index","Fitness"])
    df.Fitness -= df.Fitness[len(df.Fitness)-1]
    df['cum_sum'] = df.Fitness.cumsum()
    df['cum_perc'] = 100*df.cum_sum/df.Fitness.sum()
    
    for i in range(0, eliteSize):
        selectionResults.append(popRanked[i][0])
    for i in range(0, len(popRanked) - eliteSize):
        pick = 100*random.random()
        for i in range(0, len(popRanked)):
            if pick <= df.iat[i,3]:
                selectionResults.append(popRanked[i][0])
                break
    return selectionResults

def matingPool(population, selectionResults):
    matingpool = []
    for i in range(0, len(selectionResults)):
        index = selectionResults[i]
        matingpool.append(population[index])
    return matingpool

def breed(parent1, parent2):
    child = []
    childP1 = []
    childP2 = []
    
    geneA = int(random.random() * len(parent1))
    geneB = int(random.random() * len(parent1))
    
    startGene = min(geneA, geneB)
    endGene = max(geneA, geneB)

    for i in range(startGene, endGene):
        childP1.append(parent1[i])
        
    childP2 = [item for item in parent2 if item not in childP1]

    child = np.array(childP1 + childP2)
    return Schedule(child)

def breedPopulation(matingpool, eliteSize):
    children = []
    length = len(matingpool) - eliteSize
    pool = random.sample(matingpool, len(matingpool))

    for i in range(0,eliteSize):
        children.append(matingpool[i])
    
    for i in range(0, length):
        child = breed(pool[i], pool[len(matingpool)-i-1])
        children.append(child)
    return children

def mutate(individual, mutationRate):
    for swapped in range(len(individual)):
        if(random.random() < mutationRate):
            swapWith = int(random.random() * len(individual))
            
            event1 = individual[swapped]
            event2 = individual[swapWith]
            
            individual[swapped] = event2
            individual[swapWith] = event1
    return individual

def mutatePopulation(population, mutationRate):
    # Don't mutate the most promising schedule
    mutatedPop = [population[0]]
    
    for ind in range(1, len(population)):
        mutatedInd = mutate(population[ind], mutationRate)
        mutatedPop.append(mutatedInd)
    return mutatedPop

def nextGeneration(currentGen, eliteSize, mutationRate):
    popRanked = rankSchedules(currentGen)
    selectionResults = selection(popRanked, eliteSize)
    matingpool = matingPool(currentGen, selectionResults)
    children = breedPopulation(matingpool, eliteSize)
    nextGeneration = mutatePopulation(children, mutationRate)
    return nextGeneration

def geneticAlgorithm(popSize, eliteSize, mutationRate, generations):
    pop = initialPopulation(popSize)
    scores = [rankSchedules(pop)[0][1]]
    
    for i in range(0, generations):
        print(f'Iteration {i} score: {scores[-1]}')

        pop = nextGeneration(pop, eliteSize, mutationRate)
        scores.append(rankSchedules(pop)[0][1])

        # Can't do better than perfect
        if(scores[-1] == 1):
            break

    # plotting the points  
    plt.plot(range(len(scores)), scores) 
        
    # naming the x axis 
    plt.xlabel('generation') 
    # naming the y axis 
    plt.ylabel('score') 
        
    # giving a title to my graph 
    plt.title('Schedule Quality Over Time\n(higher is better)') 
        
    # function to show the plot 
    plt.show() 

    bestSchedIndex = rankSchedules(pop)[0][0]
    bestSched = pop[bestSchedIndex]
    return bestSched

# Read the input data
with open(r'data/rooms.yaml') as roomfile:
    room_list = yaml.load(roomfile, Loader=yaml.FullLoader)
    for k, v in room_list.items():
        Schedule.rooms.append(Room(k, v))
    
with open(r'data/minisymposia.yaml') as minifile:
    mini_list = yaml.load(minifile, Loader=yaml.FullLoader)
    ms_list = []
    for k, v in mini_list.items():
        # Strip the part out of the name
        m = re.match(r'(?P<name>.*) - Part (?P<part>I*) of (?P<parts>I*)', k)
        if m:
            k = m.group("name")
            part = len(m.group("part"))
            parts = len(m.group("parts"))
        else:
            part = 1
            parts = 1

        participants = [v.get("organizer", "")] + v.get("speakers", [])
        ms_list.append(Minisymposium(k, participants, part, parts))

minisymposia = Minisymposia(ms_list)

# Perform the scheduling
schedule = geneticAlgorithm(100, 20, 0.01, 10000)
print(schedule)
