import yaml
from haystack.document_stores import InMemoryDocumentStore
from haystack.nodes import TransformersDocumentClassifier

# Scraped yaml file
with open(r'./data/minisymposia.yaml') as minifile:  
    mini_list = yaml.load(minifile, Loader=yaml.FullLoader)

# Text file with all possible themes to predict
with open(r'./data/themes.txt') as file:  
    labels = file.readlines()
labels = [label.replace('\n','') for label in labels]

# This chunk creates a list of all of the talks in the format that Haystack needs
docs = [] 
for key, value in mini_list.items():
    content = key
    if 'abstract' in value.keys():
        content += '\n' + value['abstract']
    curr_dict = {"content": content,
                "meta": {
                    "name": key
                }}
    docs.append(curr_dict)

# Writes the talks into Haystack's document store
doc_store = InMemoryDocumentStore(use_gpu=True)
doc_store.write_documents(docs)

# Loads ML model that generates label prediction
doc_classifier = TransformersDocumentClassifier(
    model_name_or_path="cross-encoder/nli-distilroberta-base",
    task="zero-shot-classification",
    labels=labels,
    batch_size=16,
    use_gpu=True
)

# Runs all of the "documents" through the ML model 
# (significant speed up from GPU access)
classified_docs = doc_classifier.predict(doc_store)

# Adds generated label to the list from the yaml file
for document in classified_docs.get_all_documents():
    label = document.meta['classification']['label']
    name = document.meta['name']
    mini_list[name]['predicted_theme'] = label

# Saves the new yaml
with open(r'./data/minisymposia_predicted_theme.yaml', 'w') as file:
    yaml.dump(mini_list, file)

