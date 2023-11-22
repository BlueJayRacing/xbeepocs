
import random
import string

# Define the number of lines to generate
num_lines = 50

# Define the minimum and maximum line lengths
min_length = 60
max_length = 90

# Define the characters to use for generating random text
characters = string.ascii_letters + ' '

# Generate the random text and save it to a file
with open('random_text.txt', 'w') as f:
    for i in range(num_lines):
        line_length = random.randint(min_length, max_length)
        line = ''.join(random.choice(characters) for _ in range(line_length))
        f.write(line + '\n')
