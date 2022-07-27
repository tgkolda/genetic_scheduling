# Based on https://towardsdatascience.com/evolution-of-a-salesman-a-complete-genetic-algorithm-tutorial-for-python-6fe5d2b3ca35

import matplotlib.pyplot as plt 
import numpy as np
import operator
import pandas as pd
import random
import re
import yaml

class Minisymposium:
    def __init__(self, title, participants, part):
        self.title = title
        self.participants = participants
        self.part = part

class Room:
    def __init__(self, name, capacity):
        self.name = name
        self.capacity = capacity

class Schedule:
    minisymposia = []
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

    def is_possible(self):
        return self.needs_more_rooms() == 0

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
                str += f'{Schedule.minisymposia[s-1].title}: {Schedule.rooms[rid].name}\n'
                rid = rid + 1
        return str

class Fitness:
    def __init__(self, schedule):
        self.schedule = schedule
        self.rating = np.nan

    # Lower scores are better
    def schedFitness(self):
        if np.isnan(self.rating):
            self.rating = self.schedule.needs_more_rooms()
        return self.rating

def makeEventList():
    nmini = len(Schedule.minisymposia)
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
    return sorted(fitnessResults.items(), key = operator.itemgetter(1), reverse=False)

def selection(popRanked, eliteSize):
    selectionResults = []
    df = pd.DataFrame(np.array(popRanked), columns=["Index","Fitness"])
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

    child = childP1 + childP2
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
        pop = nextGeneration(pop, eliteSize, mutationRate)
        scores.append(rankSchedules(pop)[0][1])

        # Can't do better than perfect
        if(scores[-1] == 0):
            break

    # plotting the points  
    plt.plot(range(len(scores)), scores) 
        
    # naming the x axis 
    plt.xlabel('generation') 
    # naming the y axis 
    plt.ylabel('score') 
        
    # giving a title to my graph 
    plt.title('Schedule Quality Over Time') 
        
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
    for k, v in mini_list.items():
        # Strip the part out of the name
        m = re.match(r'(?P<name>.*) - Part (?P<part>I*) of I*', k)
        if m:
            k = m.group("name")

        participants = []
        participants.append(v.get("organizer", []))
        participants.append(v.get("speakers", []))
        part = v.get("part", 1)
        Schedule.minisymposia.append(Minisymposium(k, participants, part))

# Perform the scheduling
schedule = geneticAlgorithm(100, 10, 0.01, 1000)
print(schedule)
