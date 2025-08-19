import numpy as np
import math
from scipy.ndimage import gaussian_filter 
import numbers
from PIL import Image
from torchvision import transforms
import torch
from torch import Tensor


class TinyEyes(torch.nn.Module):
    """Apply TinyEyes Transformation to a given PIL Image or torch Tensor.
    
    Args:
        age (str): Postnatal age (week0, week4, ..., adult)
        width (float | int): Display width in cm
        dist (float | int): Viewing distance in cm
        imp (str): 'cpu' or 'gpu' implementation
    """
    
    _convert_rgb2lms_np=np.array([[0.05059983, 0.08585369, 0.00952420],
                               [0.01893033, 0.08925308, 0.01370054],
                               [0.00292202, 0.00975732, 0.07145979]])  
    _convert_rgb2lms_t = torch.from_numpy(_convert_rgb2lms_np).float()

    _convert_lms2opp_np=np.array([[  0.5000,     0.5,       0],
                               [ -0.6690,  0.7420, -0.0270],
                               [ -0.2120, -0.3540,  0.9110]])
    _convert_lms2opp_t = torch.from_numpy(_convert_lms2opp_np).float()

    _convert_lms2rgb_np=np.linalg.pinv(_convert_rgb2lms_np)
    _convert_lms2rgb_t = torch.from_numpy(_convert_lms2rgb_np).float()

    _convert_opp2lms_np=np.linalg.inv(_convert_lms2opp_np)
    _convert_opp2lms_t = torch.from_numpy(_convert_opp2lms_np).float()

    # Get model of BLUR
    # Values estimated by Prof Alex Wade and colleagues from developmental literature
    _modelByAge = {'week0':   np.array([0.6821,100, 1000]),
                    'week4':  np.array([0.48,4.77,100]),
                    'week8':  np.array([0.24, 2.4, 4]),
                    'week12': np.array([0.1,0.53,2]),
                    'week24': np.array([0.04,0.12,1]),
                    'adult':  np.array([0.01,0.015,0.02])}
    
    _possible_ages = ['week0', 'week4', 'week8', 'week12', 'week24', 'adult']
        

    def __init__(self, age: str,
                 width: float | int,
                 dist: float | int,
                 imp: str = 'cpu'):
        
        
        super().__init__() 
            
        if age not in TinyEyes._possible_ages:
            raise ValueError(f"Age must be one of {TinyEyes._possible_ages}")
        self.age = age

        if not isinstance(width, numbers.Number) or width <= 0:
            raise TypeError("width must be a positive number (cm)")
        self.width = width

        if not isinstance(dist, numbers.Number) or dist <= 0:
            raise TypeError("dist must be a positive number (cm)")
        self.dist = dist

        if imp not in ['cpu', 'gpu']:
            raise ValueError("imp must be 'cpu' or 'gpu'")
        self.imp = imp
        
        self.modelForAge = TinyEyes._modelByAge[age]

        self.n_images_processed = 0

    def calc_pixels_per_degree(self, px_size: int | float) -> float:
        ratio =(self.width/2)/self.dist
        t = 2 * math.degrees((math.atan(ratio)))
        ppd = px_size/t
        return ppd

    def calc_channelwise_blur(self, img: Image.Image | Tensor) -> np.ndarray:

        if isinstance(img, Image.Image):
            img_w, img_h = img.size
        elif isinstance(img, Tensor):
            img_c, img_w, img_h = img.size() if img.ndim == 3 else img.shape[1:]
        else:
            raise TypeError("img must be PIL.Image or torch.Tensor")

        if img_w != img_h:
            raise ValueError("Image must be square. Use CenterCrop first.")
        
        px_size = img_w  # Width also equals height

        # Calculate channelwise blur based on input parameters
        ppd = self.calc_pixels_per_degree(px_size)
        lrb = self.modelForAge * ppd
        channelwise_blur = np.around(lrb, 2)

        return channelwise_blur

    def forward(self, img: Image.Image | Tensor) -> Image.Image | Tensor:

        # PIL Image using CPU 
        if isinstance(img, Image.Image):
            if self.imp != "cpu":
                raise TypeError("PIL images can only be processed in 'cpu' mode.")
            channelwise_blur = self.calc_channelwise_blur(img)
            return self.blur_np_image(img, channelwise_blur)

        # Torch Tensor
        if isinstance(img, Tensor):
            # Ensure batch dimension
            if img.dim() == 3:
                img = img.unsqueeze(0)  # (C, H, W) -> (1, C, H, W)

            # Calculate blur once (assume all images in batch have same size)
            channelwise_blur = self.calc_channelwise_blur(img[0])

            # Apply blur (uses CPU or GPU depending on self.imp)
            return self.blur_torch_image(img, channelwise_blur)

        raise TypeError("Input must be a PIL.Image.Image or a torch.Tensor.")

    def blur_np_image(self, img: Image.Image, channelwise_blur: np.ndarray) -> Image.Image:
        img_np = np.array(img).astype(float)
        x, y, z = img_np.shape
        img_rgb_flat = img_np.transpose(2,0,1).reshape(3,-1)
        img_lms = np.matmul(self._convert_rgb2lms_np, (img_rgb_flat - 128)/128)
        img_opp = np.matmul(self._convert_lms2opp_np, img_lms)

        img_opp_blur = np.empty_like(img_opp)
        for i in range(3): # Hard coding 3 as that is what the TinyEyes Model uses!
            img_channel = img_opp[i, :].reshape(x, y)
            img_channel_blur = gaussian_filter(img_channel, channelwise_blur[i])
            img_opp_blur[i, :] = img_channel_blur.flatten()
            
        img_out_lms = np.matmul(self._convert_opp2lms_np, img_opp_blur)
        img_out_rgb = np.clip(np.matmul(self._convert_lms2rgb_np, img_out_lms) * 128 + 128, 0, 255)
        img_out_rgb_reshaped = img_out_rgb.reshape(3, x, y).transpose(1,2,0).astype('uint8')
        return Image.fromarray(img_out_rgb_reshaped, 'RGB')

    def _rgb_to_opp_t(self, img_flat: Tensor, device: str) -> Tensor:
        img_lms = torch.matmul(self._convert_rgb2lms_t.to(device), (img_flat - 0.5) / 0.5)
        return torch.matmul(self._convert_lms2opp_t.to(device), img_lms)

    def _opp_to_rgb_t(self, img_blur: Tensor, device: str) -> Tensor:
        img_lms = torch.matmul(self._convert_opp2lms_t.to(device), img_blur)
        img_out = torch.matmul(self._convert_lms2rgb_t.to(device), img_lms) * 0.5 + 0.5
        return torch.clip(img_out, 0, 1)

    def blur_torch_image(self, img: Tensor, channelwise_blur: np.ndarray) -> Tensor:
        device = 'cuda' if self.imp == 'gpu' else 'cpu'
        n, c, x, y = img.shape
        img_flat = img.permute(1,0,2,3).reshape(3, -1).float().to(device)
        img_opp = self._rgb_to_opp_t(img_flat, device)
        img_blur = torch.empty_like(img_opp)

        max_kernel_size = (x * 2) - 1
        for channel in range(c):
            kernel_size = max(3, min((2*round(4*channelwise_blur[channel]))+1, max_kernel_size))
            img_blur[channel, :] = transforms.GaussianBlur(kernel_size, sigma=channelwise_blur[channel])(
                img_opp[channel, :].reshape(n, x, y)
            ).view(-1)

        img_out = self._opp_to_rgb_t(img_blur, device)
        return img_out.reshape(c, n, x, y).permute(1,0,2,3)

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}(age={self.age}, width={self.width}, dist={self.dist})"