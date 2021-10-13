.title Post processing

Post processing occures after main scene rendering in order to add more realistic effects (motion blur, ambient occlusion...)

.img("post_process_pipeline.png")

## Post process
 * [BloomPostProcess]
 * [ChromaticDispersionPostProcess]
 * [HSLPostProcess]
 * [MotionBlurPostProcess]
 * [RadialBlurPostProcess]
 * [SAOPostProcess]
 * [SharpenPostProcess]
 
### Usage (python)

All post-processings have the same init/remove procedures:  


```
import harfang as hg
...
camera = scene.GetCurrentCamera()
post_process = hg.BloomPostProcess()
camera.AddComponent(post_process)

...

camera.RemoveComponent(post_process)
...
```

_In that exemple we use "BloomPostProcess", but it could be "ChromaticDispersionPostProcess", "HSLPostProcess", and so on..._
 
## Post process Stack

You can add as much PostProcessComponent as you like to the Camera.  
It will be executed in the order of appearance in the Node's stack: this is the **Post Process stack**.

The order of execution of the post-processes is important:

.img("post_process_stack.png")


---

## BloomPostProcess

Drops a glow around enlighten areas. This effect enhances the impression of brightness.  

_Bloom post-process:_  
.img("bloom_01.png")

_No post-process:_  
.img("no_post_process.png")


---

## ChromaticDispersionPostProcess

This filter works by independently offseting the red, green and blue components of the input image.

_Chromatic dispersion post-process:_  
.img("chromatic_dispersion_01.png")

_No post-process:_  
.img("no_post_process.png")

---

## HSLPostProcess

Post-process component implementing a Hue/Saturation/Brightness filter.

* **Hue:** add a circular shift to pixel colors.
* **Saturation:** set the colors strength (0: picture in grayscale).
* **Brightness:** Set the brightnes level (0 sets the screen to black). This can be used to fade-in / fade-out effect.

_HSL post-process:_  
.img("HSL_01.png")

_No post-process:_  
.img("no_post_process_camaro.png")

---

## MotioBlurPostProcess

This effect reproduce the famous cinematographic effect that blurs moving parts.  
The faster the part, the blurrier it is. Motion-blur increases the realism of the rendering by softening the movements.

_Motion blur post-process:_  
.img("motionblur_01.png")

_No post-process:_  
.img("no_post_process_camaro.png")

---

## RadialBlurPostProcess

This effect blurs the pixels from a point of the screen. The further the pixels are from the point, the blurrier they are.  
Unlike motion-blur, the radial-blur is independent of motion.

_Radial blur post-process:_  
.img("radial_blur_01.png")

_No post-process:_  
.img("no_post_process_camaro.png")

---

## SAOPostProcess

S.A.O for Screen-space Ambient Occlusion.

SAO is a fast real-time ambient occlusion rendering. It calculates pixels occlusions using the frame Z-Buffer.  
It's an approximation of real ambient occlusion, but it's quite faster.  
SAO is independant of scene complexity, as it works only on pixels datas (colors buffer & Z-Buffer).


_SAO post-process:_  
.img("SAO_01.png")

_No post-process:_  
.img("no_post_process_camaro.png")

---

## SharpenPostProcess

This effect reinforce contrasted edges using a convolution matrice.

_Sharpen post-process:_  
.img("Sharpen_01.png")

_No post-process:_  
.img("no_post_process.png")
