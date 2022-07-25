from urllib.request import urlopen
import re
import yaml

url = "https://meetings.siam.org/program.cfm?CONFCODE=CS19"
page = urlopen(url)
html = page.read().decode("utf-8")
minisymposia = re.findall("<a href=.*?>MS.*?</a>", html)

data = {}

for mini in minisymposia:
    dict = {}

    # Get the name and URL for the minisymposium
    m = re.match(r'.*?\"(?P<url>.*)\">MS.*? (?P<name>.*)</a>', mini)
    url = "https://meetings.siam.org/" + m.group("url")
    name = m.group("name")
    print(name)

    # If the name includes "- Part X of Y", strip it out
    m = re.match(r'(?P<name>.*) - Part (?P<part>I*) of I*', name)
    if m:
        dict["part"] = len(m.group("part"))

    # Open the minisymposium webpage
    page = urlopen(url)
    html = page.read().decode("utf-8")

    # Get the organizer's name
    m = re.match(r'.*?<b>Organizer:</b>.*?<b>(?P<organizer>.*?)</b>.*', html, re.DOTALL)
    if m:
        dict["organizer"] = m.group("organizer").strip()

    # Get the speakers names
    speakers = re.findall("<em>(.*?)</EM>", html)
    dict["speakers"] = [speaker.strip() for speaker in speakers]

    data[name] = dict

with open(r'minisymposia.yaml', 'w') as file:
    yaml.dump(data, file)