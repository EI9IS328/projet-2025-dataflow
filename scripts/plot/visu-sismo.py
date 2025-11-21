import matplotlib.pyplot as plt 
import sys

if len(sys.argv) < 2:
    print("Usage: python script.py <nom_du_fichier>")
    sys.exit(1)

filename = sys.argv[1]

with open(filename, 'r') as f:
    content = f.read()  # lit tout le fichier dans une string

content = content.strip()
pressureValues = content.split(' ')
pressureValues = list(map(float, pressureValues))

coords = filename.split('/')[-1].rsplit('-', 1)[0]
[x,y,z] = coords.split('-')
[x,y,z] = [float(x), float(y), float(z)]

print(f"{x} {y} {z}")



plt.plot(range(len(pressureValues)), pressureValues)
plt.title(f"Receiver x:{x}, y:{y}, z:{z}")
plt.savefig(f"plot-{filename.split('/')[-1].split('.')[0]}")
plt.show()


    