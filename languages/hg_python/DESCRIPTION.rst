.. image:: https://raw.githubusercontent.com/harfang3d/image-storage/main/brand/logo_harfang3d_horizontal-512px.png

Harfang for Python
==================

**3D real time visualization framework**

Harfang is a 3D real time visualization framework for the industry, the education and for scientists. It grants developpers the ability to create applications ranging from data visualization to games.

See https://www.harfang3d.com/license for licensing terms.

| 
| **Quickstart**

1. Download the tutorials https://github.com/harfang3d/tutorials-hg2 and unzip them to your computer (eg. *d:/tutorials-hg2*).
2. To compile the tutorial resources, download **assetc** for your platform: https://www.harfang3d.com/releases/
3. Drag and drop the tutorial resources folder on the **`assetc** executable -OR- execute **assetc** passing it the path to the tutorial resources folder (eg. *assetc d:/tutorials-hg2/resources*).

.. image:: https://raw.githubusercontent.com/harfang3d/image-storage/main/tutorials/assetc.gif

After the compilation process finishes, you should see a ``resources_compiled`` folder next to the resources folder. You can now execute the tutorials from the folder you unzipped them to.

    ``D:\tutorials-hg2>python draw_lines.py``

Alternatively you can open the tutorial folder and run the provided debug targets using `Visual Studio Code <https://code.visualstudio.com/>`_

| 
| **Screenshots**

The following screenshots were captured on a 2070RTX in 1080P running at 60FPS, GI is performed using screen space raytracing and does not require RTX capable hardware.

| 
| Cyber City *(CyberPunk City, CyberPunk Girl and Robot R32 by art-equilibrium, ILranch and ZeroArt3d)*

.. image:: https://raw.githubusercontent.com/harfang3d/image-storage/main/portfolio/3.1.1/cyber_city_aaa.png

.. image:: https://raw.githubusercontent.com/harfang3d/image-storage/main/portfolio/3.1.1/cyber_city_aaa_2.png

| 
| Sun Temple *(Sun Temple, courtesy of the Open Research Content Archive)*

.. image:: https://raw.githubusercontent.com/harfang3d/image-storage/main/portfolio/2.0.111/sun_temple_aaa.png

.. image:: https://raw.githubusercontent.com/harfang3d/image-storage/main/portfolio/2.0.111/sun_temple_aaa_2.png

| 
| Cafe Exterior *(Bistro, courtesy of the Open Research Content Archive)*

.. image:: https://raw.githubusercontent.com/harfang3d/image-storage/main/portfolio/2.0.111/cafe_exterior_aaa.png

.. image:: https://raw.githubusercontent.com/harfang3d/image-storage/main/portfolio/2.0.111/cafe_exterior_aaa_2.png

| 
| Sponza Atrium *(Sponza Atrium GLTF, courtesy of Crytek/Themaister)*

.. image:: https://raw.githubusercontent.com/harfang3d/image-storage/main/portfolio/3.1.1/sponza_atrium_aaa.png

.. image:: https://raw.githubusercontent.com/harfang3d/image-storage/main/portfolio/3.1.1/sponza_atrium_aaa_2.png

| 
| **Features**

| Scene API

* Node & component based
* Performance oriented

| Rendering pipeline

* Low-spec PBR rendering pipeline
* High-spec 'AAA' rendering pipeline (screen space GI & reflection)
* Support of user pipeline shaders

| VR API

* VR support via OpenVR/SteamVR with Eye tracking
* Compatible with the HTC Vive/Vive Pro, Valve Index, Lenovo Explorer, Oculus Rift S

| Physics API

* Rigid bodies, collisions, mechanical constraints
* Ray casting
* Fast & accurate

| Audio API

* Play/stream WAV/OGG formats
* 3D audio spatialization

More information on https://www.harfang3d.com/