import pandas as pd
import matplotlib.pyplot as plt

# Load data from CSV file with the new format
data = pd.read_csv("offset_data_with_pc.csv")

# Get unique PC values
unique_pcs = data['PC'].unique()
print(len(unique_pcs))
# Plotting for each PC
for pc in unique_pcs:
    print(pc)
    # Filter data for the current PC
    pc_data = data[data['PC'] == pc]
    
    # Filter data for green color
    green_data = pc_data[pc_data['Color'] == 'green']
    
    # Filter data for red color
    red_data = pc_data[pc_data['Color'] == 'red']

    # Plotting
    fig = plt.figure(figsize=(20, 20))
    ax1 = fig.add_subplot(111)
    size = 0.01
    if len(green_data) < 1000 & len(red_data) < 1000:
        size = 3
    # Plot red points
    ax1.scatter(red_data['offset'], red_data['Index'], color='r', s=size, linewidth=0)

    # Plot green points
    ax1.scatter(green_data['offset'], green_data['Index'], color='g', s=0.5, linewidth=0, alpha=0.5)
    
    plt.xlabel('Offset from faulting address')
    plt.ylabel('Pagefault')
    plt.title(f'Points Separated by Color for PC {pc}')
	
	# Set x-axis limits from -256 to 256
    plt.xlim(-256, 256)
    # Add legend
    #plt.legend()

    # Save plot as PDF with PC value in the filename
    plt.savefig(f"points_separated_by_color_PC_{pc}.pdf")
    plt.close()
    # Show plot
    #plt.show()

