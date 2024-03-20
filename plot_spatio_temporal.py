import pandas as pd
import matplotlib.pyplot as plt

# Load data from CSV file with the new format
data = pd.read_csv("offset_data_with_pc.csv")

# Get unique PC values
# Plotting for each PC
# Filter data for the current PC

# Filter data for green color
green_data = data[data['Color'] == 'green']

# Filter data for red color
red_data = data[data['Color'] == 'red']

# Plotting
fig = plt.figure(figsize=(20, 20))
ax1 = fig.add_subplot(111)
size = 0.01
# Plot red points
ax1.scatter(red_data['offset'], red_data['Index'], color='r', s=size, linewidth=0)

# Plot green points
ax1.scatter(green_data['offset'], green_data['Index'], color='g', s=size, linewidth=0)

plt.xlabel('Offset from faulting address')
plt.ylabel('Pagefault')
plt.title(f'Points Separated by Color entire dataset')

# Set x-axis limits from -256 to 256
plt.xlim(-256, 256)
# Add legend
#plt.legend()

# Save plot as PDF with PC value in the filename
plt.savefig(f"points_separated_by_color.pdf")
plt.close()
# Show plot
#plt.show()

