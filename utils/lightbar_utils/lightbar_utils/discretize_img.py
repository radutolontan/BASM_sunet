from PIL import Image
import numpy as np
import matplotlib.pyplot as plt
import argparse

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--imgpath', type=str, default=None, dest='imgpath',
                        help='Path to .img file')
    parser.add_argument('--xpts', type=int, default=None, dest='xpts',
                        help='No. of discrete x gridpoints')
    parser.add_argument('--ypts', type=int, default=None, dest='ypts',
                        help='No. of discrete y gridpoints')
    return parser.parse_args()

def discretize_image(image_path, x, y):
    """Resize an image and return an (x, y, 3) matrix of RGB values."""
    image = Image.open(image_path).convert("RGB")
    image_resized = image.resize((x, y), Image.Resampling.NEAREST)
    return np.array(image_resized)

def rgb_to_grb(rgb_matrix):
    """Convert an RGB matrix into a NeoPixel-compatible GRB matrix."""
    grb_matrix = rgb_matrix.copy()
    grb_matrix[:, :, 0], grb_matrix[:, :, 1] = rgb_matrix[:, :, 1], rgb_matrix[:, :, 0]

    return grb_matrix # Convert to uint32

def pack_bytes(b, c, d):
    return np.uint32((int(b) << 16) | (int(c)<< 8) | int(d))

def visualize_matrix(matrix, flipped_rgb_matrix):
    """Display the discretized image."""
    plt.figure(figsize=(6, 6))
    plt.subplot(1,2,1)
    plt.imshow(matrix)
    plt.axis("off")
    plt.title("Discretized Image")
    plt.subplot(1,2,2)
    plt.imshow(flipped_rgb_matrix)
    plt.axis("off")
    plt.title("Inverted Image")
    plt.show()

args = parse_args()

# Parameters
image_path = args.imgpath # Import image path
output_file = image_path[:-4] + ".csv" # Generate output file name
output_file = output_file[2:]
x, y = args.xpts, args.ypts  # Adjust resolution for your NeoPixel strip

# Process Image to RGB
rgb_matrix = discretize_image(image_path, x, y)

# Flip matrix about x-axis
flipped_rgb_matrix = np.flip(rgb_matrix, axis=0)

# Pack RGB values to uint32
neopixel_array = np.zeros((y, x), dtype=np.uint32)
# np.zeros((y, x), dtype=np.uint32) - y rows ; x columns
for i in range(y): # ALL ROWS i
    for j in range(x): # ALL COLUMNS J
        # Pack the RGB values into a single uint32
        # Flip about the x-Axis
        neopixel_array[i, j] = pack_bytes(b = int(flipped_rgb_matrix[i, j, 0]),
                                         c = int(flipped_rgb_matrix[i, j, 1]), 
                                         d = int(flipped_rgb_matrix[i, j, 2]))

# Display hex neopixel_array in hex
for row in neopixel_array:
    print(" ".join(f"0x{val:08X}" for val in row)) 

# Save to CSV
np.savetxt(output_file, neopixel_array, delimiter=",", fmt="%u")  # Save in uint32 format


# Visualize Image
visualize_matrix(rgb_matrix, flipped_rgb_matrix)