# TFG_Mètode_Newton

## Table of Contents
- [Getting Started](#getting-started)
    - [Downloading the Repository](#downloading-the-repository)
    - [Running the Demo](#running-the-demo)
    - [Building and Running the Project](#building-and-running-the-project)
- [How to use](#how-to-use)


## Getting Started

### Downloading the Repository
1. Navigate to the main page of the repository on GitHub.
2. Above the list of files, click the green button "Code".
3. To clone the repository using HTTPS, under "Clone with HTTPS", click the clipboard icon. To clone the repository using an SSH key, including a certificate issued by your organization's SSH certificate authority, click Use SSH, then click the clipboard icon.
4. Open Terminal.
5. Change the current working directory to the location where you want the cloned directory.
6. Type 'git clone', and then paste the URL you copied earlier.
7. Press Enter to create your local clone.

### Running the Demo
1. Navigate to the `x64` directory in the cloned repository.
2. Enter the `Debug` directory.
3. Execute the `TFG_Go.exe` file. 
4. Enjoy the experience!

### Building and Running the Project
If you wish to examine the code and run it, you would need to do so using Visual Studio:

1. Open Visual Studio.
2. Navigate to the `File` menu and select `Open`, then `Project/Solution`.
3. Navigate to the cloned repository directory and open the `TFG_GO.snl` solution file.
4. Once the solution is loaded, build and run the project as per usual in Visual Studio.

## How to use

### User Interface
- Once the program has loaded, you should see a user interface (UI) with a configuration panel on the left and the point cloud visualizer in the center.

### Point Cloud Configuration
- Under the 'Configuración' header on the left side, you will see the 'PointCloud' section.
    - In this section, you can input a file URL or select a '.ply' file from your computer to load a point cloud.
    - Additionally, you can adjust various properties of the point cloud including the number of Octrees, maximum points, maximum depth, and more. Click 'Cargar' to load the point cloud with the selected settings, or 'Borrar' to remove the currently selected point cloud.
    - Use the point cloud selector to switch between loaded point clouds.

### Camera Configuration
- Under the 'Camera' header, you can switch between free mode and fixed mode, adjust the field of view (FoV), movement speed, and sensitivity. You can also adjust the position and orientation of the camera.

### Mode Selection
- Under the 'Modes' header, you can choose between different visualization modes including voxel and octree. There are various other options available depending on the selected mode, such as enabling or disabling Phong shading, segmented view, error display, shadow display, extra view, and ordered view.

### Shading Configuration
- Under the 'Shading' header, you can adjust various shading properties such as the g, and t. You can also add and delete point lights and adjust their properties.

### Plane Display Configuration
- Under the 'Planes' header, you can enable or disable the display of planes. You can adjust the scale factor of the planes and their material properties.

### Information Display
- At the bottom of the configuration panel, you can view various statistics such as frames per second (FPS), window dimensions, camera distance, the number of nodes, etc.

Remember to click 'Cargar' to load the point cloud with the current settings after making any changes in the configuration panel. Enjoy visualizing your point clouds!
