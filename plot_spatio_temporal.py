import pandas as pd
import matplotlib.pyplot as plt

# Load data from CSV file
data = pd.read_csv("offset_data_without_pc.csv")

# Extracting data
indexes = data['Index']
offsets = data[' offset']
colors = data[' Color']

# Randomly select 200 indexes
#sample_data = data.sample(n=200).reset_index(drop=True)

# Extracting data
#indexes = sample_data['Index']
#offsets = sample_data[' offset']
#colors = sample_data[' Color']

# Plotting
plt.figure(figsize=(8, 6))

for i in range(len(indexes)):
    plt.scatter(offsets[i], indexes[i], color=colors[i], marker='o')

plt.xlabel('Offset')
plt.ylabel('Index')
plt.title('Point Plot')

plt.grid(True)
# Save plot as PDF
plt.savefig("point_plot.pdf")
plt.show()

