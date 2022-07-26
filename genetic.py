# Based on https://towardsdatascience.com/evolution-of-a-salesman-a-complete-genetic-algorithm-tutorial-for-python-6fe5d2b3ca35

import numpy as np
import operator
import random
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
    nslots = 11
    rng = np.random.default_rng()

    # eventList is a list of integers - positive are minisymposia, 0s are delimeters between timeslots
    def __init__(self, eventList):
        self.events = Schedule.rng.permutation(eventList)

    def is_possible(self):
        rid = 0 # room index
        for s in self.events:
            if s == 0:
                rid = 0
            else:
                if rid >= len(Schedule.rooms):
                    return False
                rid = rid + 1
        return True

    def __repr__(self):
        if not self.is_possible():
            return 'impossible'

        slot = 1
        rid = 0 # room index
        str = f'Slot {slot}\n'
        for s in self.events:
            if s == 0:
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

    def schedFitness(self):
        if np.isnan(self.rating):
            if self.schedule.is_possible():
                self.rating = 1
            else:
                self.rating = 0
        return self.rating

def makeEventList():
    nmini = len(Schedule.minisymposia)
    ngenes = nmini + Schedule.nslots - 1 
    eventList = np.zeros(ngenes, dtype=np.int32)
    for i in range(nmini):
        eventList[i] = i+1
    return eventList

def initialPopulation(popSize):
    eventList = makeEventList()
    pop = []
    for i in range(popSize):
        pop.append(Schedule(eventList))
    return pop

def nextGeneration(pop, eliteSize, mutationRate):
    return pop

def rankSchedules(population):
    fitnessResults = {}
    for i in range(0,len(population)):
        fitnessResults[i] = Fitness(population[i]).schedFitness()
    return sorted(fitnessResults.items(), key = operator.itemgetter(1))

def geneticAlgorithm(popSize, eliteSize, mutationRate, generations):
    pop = initialPopulation(popSize)
    print("Initial score: " + str(rankSchedules(pop)[0][1]))
    
    for i in range(0, generations):
        pop = nextGeneration(pop, eliteSize, mutationRate)
    
    print("Final score: " + str(rankSchedules(pop)[0][1]))
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
        participants = []
        participants.append(v.get("organizer", []))
        participants.append(v.get("speakers", []))
        part = v.get("part", 1)
        Schedule.minisymposia.append(Minisymposium(k, participants, part))

schedule = geneticAlgorithm(50, 10, 0.1, 10)
print(schedule)
