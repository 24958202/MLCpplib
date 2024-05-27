import csv
import mysql.connector
import json

def escape_special_characters(input_string):
    special_characters = {
        "'": "\\'",
        '"': '\\"',
        '\\': '\\\\',
        '\n': '\\n',
        '\r': '\\r',
        '\t': '\\t',
        '\b': '\\b',
        '\0': '\\0',
        '\x1a': '\\Z'  # ASCII 26 (EOF)
    }

    escaped_string = ''
    for char in input_string:
        if char in special_characters:
            escaped_string += special_characters[char]
        else:
            escaped_string += char

    return escaped_string

def connect_to_mysql():
    db = mysql.connector.connect(
        host='localhost',
        user='24958202',
        password='7122759',
        database='nlp_db'
    )

    if db.is_connected():
        print("Connected to MySQL database")
    else:
        print("Error connecting to MySQL database")

def read_csv_file(filename):
    data = []
    with open(filename, 'r') as file:
        reader = csv.reader(file)
        for row in reader:
            data.append(row)

    return data

def insert_data_into_mysql(data):
    db = mysql.connector.connect(
        host='localhost',
        user='24958202',
        password='7122759',
        database='nlp_db'
    )
    cursor = db.cursor()

    for row in data[1:]:  # Skip header row
        if len(row) < 4:
            print("Skipping row due to insufficient columns: ", row)
            continue

        word = row[0].strip()
        word_type = row[1].strip()
        english = row[2].strip()
        zh = row[3].strip()

        word = word.translate(str.maketrans({"'": "'"})).strip()
        word = word.replace(",", "")

        query = "INSERT INTO english_voc (word, word_type, meaning_en, meaning_zh) VALUES (%s, %s, %s, %s)"
        cursor.execute(query, (word, word_type, english, zh))

    db.commit()
    print("Data successfully inserted into MySQL database!")

    cursor.close()
    db.close()

def main():
    filename = "/home/ronnieji/watchdog/db/complete_db_export.csv"
    connect_to_mysql()
    data = read_csv_file(filename)
    insert_data_into_mysql(data)

if __name__ == '__main__':
    main()
