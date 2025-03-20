from PIL import Image
import numpy as np
import matplotlib.pyplot as plt

def discretize_image(image_path, x, y):
    """Resize an image and return an (x, y, 3) matrix of RGB values."""
    image = Image.open(image_path).convert("RGB")
    image_resized = image.resize((x, y), Image.Resampling.NEAREST)
    return np.array(image_resized)

def rgb_to_grb(rgb_matrix):
    """Convert an RGB matrix into a NeoPixel-compatible uint32 GRB matrix."""
    # r, g, b = rgb_matrix[..., 0], rgb_matrix[..., 1], rgb_matrix[..., 2]
    grb_matrix = rgb_matrix.copy()
    grb_matrix[:, :, 0], grb_matrix[:, :, 1] = rgb_matrix[:, :, 1], rgb_matrix[:, :, 0]

    return grb_matrix # Convert to uint32

def pack_bytes(b, c, d):
    return np.uint32((int(b) << 16) | (int(c)<< 8) | int(d))

def visualize_matrix(matrix):
    """Display the discretized image."""
    plt.figure(figsize=(6, 6))
    plt.imshow(matrix)
    plt.axis("off")
    plt.title("Discretized Image")
    plt.show()

# Parameters
image_path = "sunset.jpg"  # Replace with your actual image file
x, y = 8, 30  # Adjust resolution for your NeoPixel strip

# Process Image to RGB
rgb_matrix = discretize_image(image_path, x, y)

# Compute NeoPixel GRB matrix
grb_matrix = rgb_to_grb(rgb_matrix)

# Pack GRB values to uint32
neopixel_array = np.zeros((y, x), dtype=np.uint32)
# np.zeros((y, x), dtype=np.uint32) - y rows ; x columns
for i in range(y): # ALL ROWS i
    for j in range(x): # ALL COLUMNS J
        # Pack the GRB values into a single uint32
        neopixel_array[i, j] = pack_bytes(b = int(grb_matrix[i, j, 0]),
                                         c = int(grb_matrix[i, j, 1]), 
                                         d = int(grb_matrix[i, j, 2]))

# Display hex neopixel_array in hex
for row in neopixel_array:
    print(" ".join(f"0x{val:08X}" for val in row)) 

# Save to CSV
np.savetxt("sunset.csv", neopixel_array, delimiter=",", fmt="%u")  # Save in uint32 format


# Visualize Image
visualize_matrix(rgb_matrix)