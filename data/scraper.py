from urllib.request import urlopen
import re
import yaml

url = "https://meetings.siam.org/program.cfm?CONFCODE=CS19"
page = urlopen(url)
html = page.read().decode("utf-8")
minisymposia = re.findall("<a href=.*?>\s*MS.*?</a>", html)
panels = re.findall("<a href=.*?>\s*CP.*?</a>", html)

data = {}

for mini in minisymposia:
    dict = {}

    # Get the name and URL for the minisymposium
    m = re.match(r'.*?\"(?P<url>.*)\">\s*MS.*? (?P<name>.*)</a>', mini)
    url = "https://meetings.siam.org/" + m.group("url")
    name = m.group("name").strip(' \'"')
    print(name)

    # If the name includes "- Part X of Y", strip it out
    m = re.match(r'(?P<name>.*) - Part (?P<part>I*) of I*', name)
    if m:
        dict["part"] = len(m.group("part"))

    # Open the minisymposium webpage
    page = urlopen(url)
    html = page.read().decode("utf-8")

    # Get the abstract
    m = re.match(r'.*<p>(?P<abstract>.*?)<P>.*?<b>Organizer:</b>.*', html, re.DOTALL)
    if m:
        dict["abstract"] = m.group("abstract").strip()

    # Get the organizer's name
    m = re.match(r'.*?<b>Organizer:</b>.*?<b>(?P<organizer>.*?)</b>.*', html, re.DOTALL)
    if m:
        dict["organizer"] = m.group("organizer").strip()

    # Get the speakers names
    speakers = re.findall("<em>(.*?)</EM>", html)
    dict["speakers"] = [speaker.strip() for speaker in speakers]

    data[name] = dict

for mini in panels:
    dict = {}

    # Get the name and URL for the minisymposium
    m = re.match(r'.*?\"(?P<url>.*)\">\s*CP.*? (?P<name>.*)</a>', mini)
    url = "https://meetings.siam.org/" + m.group("url")
    name = m.group("name").strip(' \'"')
    print(name)

    # If the name includes "- Part X of Y", strip it out
    m = re.match(r'(?P<name>.*) - Part (?P<part>I*) of I*', name)
    if m:
        dict["part"] = len(m.group("part"))

    # Open the minisymposium webpage
    page = urlopen(url)
    html = page.read().decode("utf-8")

    # Get the abstract
    m = re.match(r'.*<p>(?P<abstract>.*?)<P>.*?<b>Organizer:</b>.*', html, re.DOTALL)
    if m:
        dict["abstract"] = m.group("abstract").strip()

    # Get the organizer's name
    m = re.match(r'.*?<b>Chair:</b>.*?<b>(?P<organizer>.*?)</b>.*', html, re.DOTALL)
    if m:
        dict["organizer"] = m.group("organizer").strip()

    # Get the speakers names
    speakers = re.findall("<em>(.*?)</EM>", html)
    dict["speakers"] = [speaker.strip() for speaker in speakers]

    data[name] = dict

with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/minisymposia.yaml', 'w') as file:
    yaml.dump(data, file)