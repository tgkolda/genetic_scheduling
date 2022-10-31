import gensim, nltk, numpy as np, string, yaml

NCLUSTERS = 20

# Set the words that show up in a lot of minisymposia
COMMON_WORDS = ['advanc', 'activ', 'algorithm', 'applic', 'approach', 'challeng', 'confer', 'develop', 'discuss', 'includ', 'method', 'minisymposium', 'new', 'numer', 'problem', 'process', 'recent', 'research', 'session', 'solut', 'solver', 'theori', 'use', 'workshop']

ACRONYMS = [('AI', 'Artificial Intelligence'),
            ('AMR', 'Adaptive Mesh Refinement'),
            ('BEM', 'Block Element Modifier'),
            ('BGCE', 'Bavarian Graduate School of Computational Engineering'),
            ('CFD', 'Computational Fluid Dynamics'),
            ('CSE', 'Computational Science and Engineering'),
            ('DSL', 'Domain Specific Language'),
            ('FD', 'Finite Difference'),
            ('FEM', 'Finite Element Method'),
            ('FFT', 'Fast Fourier Transform'),
            ('GPU', 'Graphical Processing Unit'),
            ('HPC', 'High Performance Computing'),
            ('LGBT', "Lesbian Gay Bisexual and Transgender"),
            ('LGBTQ', "Lesbian Gay Bisequal Transgender and Queer"),
            ('MHD', 'Magnetohydrodynamics'),
            ('MISMC', 'Multi Index Sequential Monte Carlo'),
            ('ML', 'Machine Learning'),
            ('MOR', 'Mathematics of Operations Research'),
            ('NICAM', 'Near Instantaneous Companded Audio Multiplex'),
            ('NISQ', 'Noisy Intermediate Scale Quantum'),
            ('ODE', 'Ordinary Differential Equation'),
            ('OED', 'Optimal Experimental Design'),
            ('PDE', 'Partial Differential Equation'),
            ('RBF', 'Radial Basis Function'),
            ('RDM', 'Research Data Management'),
            ('ROM', 'Reduced Order Modelling'),
            ('RSE', 'Researchers Scientists and Engineers'),
            ('SBP', 'Summation By Parts'),
            ('SQP', 'Sequential Quadratic Programming'),
            ('SVD', 'Singular Value Decomposition'),
            ('UQ', 'Uncertainty Quantification')]

nltk.download('punkt')
nltk.download("stopwords")
nltk.download('words')
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
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/minisymposia.yaml') as minifile:
    mini_list = yaml.load(minifile, Loader=yaml.FullLoader)

# Holds the tokens for each title/abstract
titles = []
tokens = []
for key, value in mini_list.items():
    titles.append(key)

    # Get the title without the Part
    title = key.split(' - ')[0]

    # Weight the minisymposia title twice as high as the talk titles
    content = title + title + ' '.join(value['talks'])

    # Replace the acronyms
    for acro in ACRONYMS:
        content = content.replace(acro[0], acro[1])

    # Remove punctuation
    content = content.translate(str.maketrans(string.punctuation, ' '*len(string.punctuation)))

    # Find acronyms
    for word in word_tokenize(content):
        if len(word) > 1 and word == word.upper():
            print(word)

    # Tokenize the title/abstract
    words = word_tokenize(content.lower())

    # Filter out the stop words
    filtered_list = [word for word in words if word not in stop_words]

    # Compute the stems of the words
    stemmed_words = [stemmer.stem(word) for word in filtered_list]

    # Filter out the stop words again (since some are stems)
    filtered_list = [word for word in stemmed_words if word not in stop_words]
    tokens.append(filtered_list)

# Create a dictionary of words found in the program
dictionary = gensim.corpora.Dictionary(tokens)

# Compute the number of times each word appears in each abstract
corpus = [dictionary.doc2bow(token) for token in tokens]

# Lower the weight of common words
tf_idf = gensim.models.TfidfModel(corpus)

# Create the similarity measure object
sims = gensim.similarities.MatrixSimilarity(tf_idf[corpus], num_features=len(dictionary))

# Use k-means to cluster the data
kmeans = KMeans(n_clusters=NCLUSTERS).fit(sims.index)

for i in range(NCLUSTERS):
    cluster = []
    for j in range(len(titles)):
        if kmeans.labels_[j] == i:
            cluster.extend(tokens[j])

    # Try to figure out what the theme is
    frequency_distribution = FreqDist(cluster)
    print("\nCLUSTER ", i+1, frequency_distribution.most_common(10))
    predicted_theme = str(i)
    for s in frequency_distribution.most_common(5):
        predicted_theme += ' ' + s[0]

    for j in range(len(titles)):
        if kmeans.labels_[j] == i:
            print(titles[j])

    # Save the theme to the associated minisymposia
    for j in range(len(titles)):
        if kmeans.labels_[j] == i:
            mini_list[titles[j]]['predicted_theme'] = predicted_theme
            
# Saves the new yaml
with open(r'//wsl.localhost/Ubuntu/home/amklinv/genetic_scheduling/data/SIAM-CSE23/minisymposia.yaml', 'w') as file:
    yaml.dump(mini_list, file)