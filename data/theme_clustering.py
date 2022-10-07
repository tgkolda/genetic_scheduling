import gensim, nltk, numpy as np, string, yaml

NCLUSTERS = 20

# Set the words that show up in a lot of minisymposia
COMMON_WORDS = ['advanc', 'activ', 'algorithm', 'applic', 'approach', 'challeng', 'confer', 'develop', 'discuss', 'includ', 'method', 'minisymposium', 'new', 'problem', 'process', 'recent', 'research', 'session', 'solut', 'use', 'workshop']

nltk.download('punkt')
nltk.download("stopwords")
from nltk import FreqDist
from nltk.corpus import stopwords
from nltk.stem import PorterStemmer
from nltk.tokenize import word_tokenize
from sklearn.cluster import KMeans

# Grab the words we want to filter out (stop words)
stop_words = set(stopwords.words("english") + COMMON_WORDS)

# Create the stemmer
stemmer = PorterStemmer()

# Read yaml file
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/minisymposia_predicted_popularity.yaml') as minifile:
    mini_list = yaml.load(minifile, Loader=yaml.FullLoader)

# Holds the tokens for each title/abstract
titles = []
abstract_docs = []
for key, value in mini_list.items():
    titles.append(key)
    content = key.split(' - ')[0]
    if 'abstract' in value.keys():
        content += '\n' + value['abstract']

    # Remove punctuation
    content = content.translate(str.maketrans('', '', string.punctuation))

    # Tokenize the title/abstract
    words = word_tokenize(content)

    # Filter out the stop words
    filtered_list = [word for word in words if word not in stop_words]

    # Compute the stems of the words
    stemmed_words = [stemmer.stem(word) for word in filtered_list]

    # Filter out the stop words again (since some are stems)
    filtered_list = [word for word in stemmed_words if word not in stop_words]
    abstract_docs.append(filtered_list)

# Create a dictionary of words found in the program
dictionary = gensim.corpora.Dictionary(abstract_docs)

# Compute the number of times each word appears in each abstract
corpus = [dictionary.doc2bow(abstract) for abstract in abstract_docs]

# Lower the weight of common words
tf_idf = gensim.models.TfidfModel(corpus)

# Create the similarity measure object
sims = gensim.similarities.MatrixSimilarity(tf_idf[corpus], num_features=len(dictionary))

# Use k-means to cluster the data
kmeans = KMeans(n_clusters=NCLUSTERS).fit(sims.index)

for i in range(NCLUSTERS):
    print("CLUSTER ", i)
    cluster = []
    for j in range(len(titles)):
        if kmeans.labels_[j] == i:
            print(titles[j])
            cluster.extend(abstract_docs[j])

    # Try to figure out what the theme is
    frequency_distribution = FreqDist(cluster)
    print(frequency_distribution.most_common(5))
    predicted_theme = str(i)
    for s in frequency_distribution.most_common(5):
        predicted_theme += ' ' + s[0]

    # Save the theme to the associated minisymposia
    for j in range(len(titles)):
        if kmeans.labels_[j] == i:
            mini_list[titles[j]]['predicted_theme'] = predicted_theme
            
# Saves the new yaml
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/minisymposia_improved_themes.yaml', 'w') as file:
    yaml.dump(mini_list, file)