import pandas as pd
import datashader as ds
import datashader.transfer_functions as tf

# Load data from CSV file
data = pd.read_csv("offset_data.csv")

# Convert 'Color' column to categorical
data['Color'] = data[' Color'].astype('category')
# Create canvas
cvs = ds.Canvas(plot_width=8000, plot_height=8000, x_range=(-256, 256), y_range=(0,200000), x_axis_type='linear', y_axis_type='linear')

# Create DataFrame
df = pd.DataFrame({'x': data[' offset'], 'y': data['Index'], 'color': data['Color']})

# Aggregate points
agg = cvs.points(df, 'x', 'y', ds.count_cat('color'))

custom_color_key = dict({'green' : 'green', 'red' : 'red'})
# Render
img = tf.shade(agg, color_key = custom_color_key)

# Save plot as PNG (DataShader outputs an image)
img.to_pil().save("point_plot_entire_dataset.png")

