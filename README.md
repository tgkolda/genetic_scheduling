# genetic_scheduling
A genetic scheduling code for SIAM CSE23

## Considerations
* Speakers can not be in two minisymposia at the same time. We should attempt to prevent coauthors from being in two minisymposia at the same time, but that is less important.
* Minisymposia with the same keywords should not take place at the same time. A person who wants to attend a linear algebra presentation will likely want to attend other linear algebra presentations
* Some minisymposia have multiple sessions which cannot take place at the same time; these are denoted by the word 'part'.
* The rooms have dramatically different sizes. The minisymposia have an additional piece of metadata estimating their popularity so the rooms can be assigned accordingly. The popularity is estimated by querying citation counts from google scholar.
* Minitutorials are being treated as minisymposia, but they may require additional equipment in the rooms (such as tables/desks). 
