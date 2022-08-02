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
    def __init__(self, title, participants, theme, part=1, parts=1):
        self.title = title
        self.participants = set(participants)
        self.theme = theme.strip()
        self.part = part
        self.parts = parts

    def same_theme(self, mini):
        return self.theme == mini.theme

    def overlaps_participants(self, mini):
        return self.participants & mini.participants

    def goes_before(self, mini):
        if self.title == mini.title:
            return self.part < mini.part
        return False

class Minisymposia:
    def __init__(self, mini_list, themes):
        self.mini_list = mini_list

        self.themes = {}
        for i in range(len(themes)):
            self.themes[themes[i].strip()] = i

        # Determine the speaker conflicts
        nmini = len(mini_list)
        self.conflicts = np.zeros((nmini, nmini), dtype=np.bool8)
        self.theme_overlap = np.zeros((nmini, nmini), dtype=np.bool8)
        self.ordering = [ [] for _ in range(nmini) ]

        # Determine the speaker conflicts and talks with a necessary ordering
        comb = combinations(range(nmini), 2)
        for m1, m2 in list(comb):
            self.conflicts[m1, m2] = mini_list[m1].overlaps_participants(mini_list[m2])
            self.conflicts[m2, m1] = self.conflicts[m1, m2]

            self.theme_overlap[m1, m2] = mini_list[m1].same_theme(mini_list[m2])
            self.theme_overlap[m2, m1] = self.theme_overlap[m1, m2]

            if mini_list[m1].goes_before(mini_list[m2]):
                self.ordering[m1].append(m2)
            elif mini_list[m2].goes_before(mini_list[m1]):
                self.ordering[m2].append(m1)

        self.nconflicts = np.count_nonzero(self.conflicts)/2
        self.overlap_penalty = self.compute_overlap_penalty()
        self.nprereqs = sum( [ len(listElem) for listElem in self.ordering])

    def __len__(self):
        return len(self.mini_list)

    def __getitem__(self, item):
        return self.mini_list[item]

    def compute_overlap_penalty(self):
        nthemes = len(self.themes)
        pen_per_topic = np.zeros((nthemes), dtype=np.int32)
        for m in self.mini_list:
            tid = self.themes[m.theme]
            pen_per_topic[tid] += 1
        for i in range(nthemes):
            pen_per_topic[i] = pen_per_topic[i]**2
        return np.sum(pen_per_topic)


class Room:
    def __init__(self, name, capacity):
        self.name = name
        self.capacity = capacity

class Schedule:
    rooms = []
    nslots = 35
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
        self.imperfections = np.zeros_like(self.events)
        nthemes = len(minisymposia.themes)
        self.overlaps = np.zeros((self.nslots, nthemes), dtype=np.int32)
        self.nproblems = 0
        self.noverlaps = 0

        # Whether the problems array has been set
        self.pset = False

    def __len__(self):
        return self.events.size

    def reset_problems(self):
        self.problems.fill(0)
        self.imperfections.fill(0)
        self.overlaps.fill(0)
        self.nproblems = 0
        self.noverlaps = 0
        self.pset = False

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

    def increment_overlap(self, slot, theme):
        tid = minisymposia.themes[theme]
        self.overlaps[slot, tid] += 1

    def overlaps_themes(self):
        for slot in range(self.nslots):
            comb = combinations(range(len(self.rooms)), 2)
            for r1, r2 in list(comb):
                m1 = self.events[slot,r1]
                m2 = self.events[slot,r2]
                if m1 < 0 or m2 < 0:
                    continue
                if(minisymposia.theme_overlap[m1, m2]):
                    self.imperfections[slot, r1] += 1
                    self.imperfections[slot, r2] += 1
                    self.increment_overlap(slot, minisymposia[m1].theme)

    def get_theme_penalty(self):
        penalty = 0
        for slot in range(self.nslots):
            for theme in range(len(minisymposia.themes)):
                penalty += (self.overlaps[slot, theme]**2)
        return penalty

    def find_problems(self):
        if not self.pset:
            self.violates_order()
            self.oversubscribes_participant()
            self.overlaps_themes()
            self.nproblems = np.sum(self.problems)
            self.noverlaps = np.sum(self.overlaps)
            self.pset = True
        return self.nproblems, self.noverlaps

    def get_random_problem(self):
        self.find_problems()
        pids = np.nonzero(self.problems)
        iids = np.nonzero(self.imperfections)
        nproblems = len(pids[0])
        nimperfections = len(iids[0])
        if nproblems > 0:
            index = Schedule.rng.choice(nproblems)
            slot = pids[0][index]
            room = pids[1][index]
        elif nimperfections > 0:
            index = Schedule.rng.choice(nimperfections)
            slot = iids[0][index]
            room = iids[1][index]
        else:
            room = Schedule.rng.choice(len(Schedule.rooms))
            slot = Schedule.rng.choice(Schedule.nslots)
        ind = (slot, room)
        return ind

    def is_possible(self):
        nproblems, _ = self.find_problems()
        return nproblems == 0

    def is_optimal(self):
        _, noverlaps = self.find_problems()
        return noverlaps == 0

    def __repr__(self):
        if not self.is_possible():
            str = f'This schedule has {self.find_problems()} problems\n'
            return str
        elif not self.is_optimal():
            _, noverlaps = self.find_problems()
            str = f'This schedule has {noverlaps} overlaps\n'
            return str
        else:
            str = ''

        for slot in range(self.nslots):
            str += f'Slot {slot}\n'
            for room in range(len(self.rooms)):
                e = self.events[slot, room]
                if e >= 0:
                    str += f'{minisymposia[e].title} ({minisymposia[e].theme}): {Schedule.rooms[room].name}\n'

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
            # Maximum penalty for overlap violations is 1 (so that multiple themes at the same 
            # time is never worse than something that makes the schedule literally impossible)
            max_penalty = minisymposia.nconflicts + minisymposia.nprereqs + 1
            nproblems, noverlaps = self.schedule.find_problems()
            cur_penalty = nproblems + self.schedule.get_theme_penalty() / minisymposia.overlap_penalty
            self.rating = 1 - cur_penalty / max_penalty
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
    for slot in range(Schedule.nslots):
        for room in range(len(Schedule.rooms)):
            event1 = individual.events[slot, room]
            if(random.random() < mutationRate):
                # Swap with something problematic
                ind = individual.get_random_problem()
                individual.events[slot, room], individual.events[ind] = individual.events[ind], individual.events[slot, room]
    return individual

def mutatePopulation(population, mutationRate):
    # Keep the best schedule
    mutatedPop = [population[0]]
    
    for ind in range(1, len(population)):
        mutatedInd = mutate(population[ind], mutationRate)
        mutatedInd.reset_problems()
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

with open('data/themes.txt') as f:
    themes = f.readlines()
    
with open(r'data/minisymposia_predicted_theme.yaml') as minifile:
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
        theme = v.get("predicted_theme", "")
        ms_list.append(Minisymposium(k, participants, theme, part, parts))

minisymposia = Minisymposia(ms_list, themes)

# Perform the scheduling
schedule = geneticAlgorithm(100, 20, 0.01, 1000)
print(schedule)
