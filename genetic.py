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

    def __getitem__(self, item):
        return self.mini_list[item]

class Room:
    def __init__(self, name, capacity):
        self.name = name
        self.capacity = capacity

class Schedule:
    rooms = []
    nslots = 13
    rng = np.random.default_rng()

    def __init__(self, eventList, permute=False):
        if permute:
            self.events = Schedule.rng.permutation(eventList)
        else:
            self.events = eventList

        # Reshape the events as a 2D array
        self.events = np.reshape(self.events, (self.nslots, len(Schedule.rooms)))

        # Store an array of how problematic a particular event assignment is
        self.problems = np.zeros_like(self.events)
        self.nproblems = 0

        # Whether the problems array has been set
        self.pset = False

    def __len__(self):
        return self.events.size

    def oversubscribes_participant(self):
        for slot in range(self.nslots):
            comb = combinations(range(len(self.rooms)), 2)
            for r1, r2 in list(comb):
                m1 = self.events[slot,r1]
                m2 = self.events[slot,r2]
                if m1 < 0 or m2 < 0:
                    continue
                if(minisymposia.conflicts[m1, m2]):
                    self.problems[slot, r1] += 1
                    self.problems[slot, r2] += 1

    def comes_before(self, first, second):
        t1= np.where(self.events == first)
        slot1 = t1[0][0]
        t2 = np.where(self.events == second)
        slot2 = t2[0][0]

        # We're going to invisibly fix the problem if possible
        if slot1 > slot2:
            self.events[t1], self.events[t2] = self.events[t2], self.events[t1]
        elif slot1 == slot2:
            self.problems[t1] += 1
            self.problems[t2] += 1

    def violates_order(self):
        for i in range(len(minisymposia)):
            for j in minisymposia.ordering[i]:
                self.comes_before(i, j)

    def find_problems(self):
        if not self.pset:
            self.violates_order()
            self.oversubscribes_participant()
            self.nproblems = np.sum(self.problems)
            self.pset = True
        return self.nproblems

    def is_possible(self):
        return self.find_problems() == 0

    def __repr__(self):
        if not self.is_possible():
            str = f'This schedule has {self.find_problems()} problems\n'
        else:
            str = ''

        return str

        for slot in range(self.nslots):
            str += f'Slot {slot}\n'
            for room in range(len(self.rooms)):
                e = self.events[slot, room]
                if e >= 0:
                    str += f'{minisymposia[e].title}: {Schedule.rooms[room].name}\n'

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
            # Maximum penalty for oversubscribing participants is the total number of conflicts
            # Maximum penalty for order violations is the total number of prerequisites
            max_penalty = minisymposia.nconflicts + minisymposia.nprereqs
            nproblems = self.schedule.find_problems()
            self.rating = 1 - nproblems / max_penalty
        return self.rating

def makeEventList():
    nmini = len(minisymposia)
    ngenes = Schedule.nslots * len(Schedule.rooms)
    eventList = np.zeros(ngenes, dtype=np.int32)
    for i in range(nmini):
        eventList[i] = i
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

# TODO: This is using lists, so we can probably improve the efficiency with numpy arrays
def breed(parent1, parent2):
    child = []
    childP1 = []
    childP2 = []
    
    geneA = int(random.random() * len(parent1))
    geneB = int(random.random() * len(parent1))
    
    startGene = min(geneA, geneB)
    endGene = max(geneA, geneB)

    p1 = np.reshape(parent1.events, -1)
    p2 = np.reshape(parent2.events, -1)

    for i in range(startGene, endGene):
        childP1.append(p1[i])
        
    childP2 = [item for item in p2 if item not in childP1]

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
    ind = np.reshape(individual.events, -1)
    for swapped in range(len(individual)):
        event1 = ind[swapped]
        if(random.random() < mutationRate):
            # Don't swap two empty rooms
            if event1 < 0:
                event2 = -1
                while event2 < 0:
                    swapWith = int(random.random() * len(individual))
                    event2 = ind[swapWith]
            else:
                swapWith = int(random.random() * len(individual))
                event2 = ind[swapWith]

            ind[swapped] = event2
            ind[swapWith] = event1
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

        bestSchedIndex = rankSchedules(pop)[0][0]
        bestSched = pop[bestSchedIndex]
        print(bestSched)

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
schedule = geneticAlgorithm(100, 20, 0.01, 1000)
print(schedule)
