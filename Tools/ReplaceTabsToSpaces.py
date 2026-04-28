import os

def replace_tabs_in_file(path):
    try:
        with open(path, 'r', encoding='utf-8') as f:
            data = f.read()
        data = data.replace('\t', '    ')
        with open(path, 'w', encoding='utf-8') as f:
            f.write(data)
        print(f"Обработан: {path}")
    except:
        pass  # Можно добавить лог

base_dir = os.getcwd()

for root, dirs, files in os.walk(base_dir):
    for file in files:
        full_path = os.path.join(root, file)
        replace_tabs_in_file(full_path)
