from TinyEyesTransform import TinyEyes as tinyeyes
from PIL import Image
from PIL import ImageOps
import os


# # Set TinyEyes Parameters
available_ages = ['week0', 'week4', 'week8', 'week12', 'week24', 'adult']

# Set TinyEyes Parameters
age = 'week8' #' Age in Weeks, choose from available_ages
w= 29 # Width in cm
d= 27 # Distance in cm

# Not a TinyEyes Parameter, just for the dataloader => arbitrary
crop_size = 224 # in pixels 

if age not in available_ages:
    raise ValueError(f"Age '{age}' is not a valid option. Choose from {available_ages}.")

# Set Input and Output Image Paths
cwd = os.getcwd()
image_dir = cwd
images = ["Well-clothed_baby.jpg"]
out_path = os.path.join(cwd, 'tinyeyes_images')

if not os.path.exists(out_path):
    os.makedirs(out_path)

te = tinyeyes(age, width=w, dist=d, imp='cpu')

for image_file in images:
    image = os.path.join(image_dir, image_file)
    image_name, _ = os.path.splitext(os.path.basename(image))

    stim_img = Image.open(image)
    stim_img = stim_img.convert('RGB') if stim_img.mode != 'RGB' else stim_img

    stim_img = ImageOps.fit(stim_img, (crop_size, crop_size), method=Image.Resampling.LANCZOS, centering=(0.5, 0.5))
    transformed_img = te(stim_img)

    out_image_name = f'{image_name}_tinyeyes_{age}_{str(w).replace(".", "pt")}w_{str(d).replace(".", "pt")}d.png'
    transformed_img.save(os.path.join(out_path,f'{out_image_name}'))












