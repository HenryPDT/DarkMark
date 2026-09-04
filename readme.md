# What is DarkMark?

[![DarkMark and DarkHelp demo](src-dox/darkmark_demo_thumbnail.png)](https://www.youtube.com/watch?v=w1lTCO2Kmsc)

DarkMark is a C++ GUI tool used to annotate images for use in neural networks.  It was written specifically to be used with the [Darknet](https://github.com/AlexeyAB/darknet) neural network framework, and has several features tailored for use with Darknet and YOLO.

![DarkMark editor window with annotated image of a dog](src-dox/darkmark_editor.png)

When you first launch DarkMark, you can specify a Darknet-style neural network to load with the selected project.  DarkMark uses that neural network to assist you in marking up more images.

![DarkMark launcher](src-dox/darkmark_launcher.png)

Several different review capabilities exist to quickly review all the annotations and highlight some common errors.

![DarkMark review window](src-dox/darkmark_review.png)

Once ready, DarkMark can also be used to generate all of the Darknet and YOLO (or other) configuration files to train a new neural network.  This includes the modifications needed to the .cfg file, as well as the .data, training and validation .txt files.  DarkMark will also create some shell scripts to start the training and copy the necessary files between computers.

![Darknet configuration](src-dox/darknet_options_partial.png)

# License

DarkMark is open source and published using the GNU GPL v3 license.  See license.txt for details.

# How to Build DarkMark

Extremely simple easy-to-follow tutorial on how to build [Darknet](https://github.com/hank-ai/darknet#table-of-contents), [DarkHelp](https://github.com/stephanecharette/DarkHelp#building-darkhelp-linux), and DarkMark:

[![DarkHelp build tutorial](https://github.com/hank-ai/darknet/raw/master/doc/linux_build_thumbnail.jpg)](https://www.youtube.com/watch?v=WTT1s8JjLFk)

DarkMark requires both [Darknet](https://github.com/hank-ai/darknet#linux-cmake-method) and [DarkHelp](https://github.com/stephanecharette/DarkHelp#building-darkhelp-linux) to build.

Once Darknet and DarkHelp have been built and installed, run the following commands to build DarkMark on Ubuntu:

    sudo apt-get install build-essential libopencv-dev libx11-dev libfreetype6-dev libxrandr-dev libxinerama-dev libxcursor-dev libmagic-dev libpoppler-cpp-dev
    cd ~/src
    git clone https://github.com/stephanecharette/DarkMark.git
    cd DarkMark
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc) package
    sudo dpkg -i darkmark*.deb

If you are using WSL2, Docker, or a Linux distro that does not come with the default fonts typically found on Ubuntu, you'll also need to install this:

    sudo apt-get install fonts-liberation

### Understanding the Build Commands

* **`cmake -DCMAKE_BUILD_TYPE=Release ..`**:
  Configures the build. CMake checks system dependencies and generates the `Makefile`. Setting `-DCMAKE_BUILD_TYPE=Release` enables compiler optimizations (`-O3`) for fast performance. You only need to run this once or when `CMakeLists.txt` changes.
* **`make -j$(nproc) package`**:
  Compiles the codebase using all CPU cores (`-j$(nproc)`) and bundles the executable, icons, and desktop entries into a Debian package (`.deb`).
* **`sudo dpkg -i darkmark*.deb`**:
  Installs the generated package system-wide so DarkMark is accessible from application menus and any terminal path.

### Development vs. Installation Workflows

* **For active development / testing changes:**
  You do not need to rebuild the `.deb` package and run `sudo dpkg -i` on every edit. Instead, compile incrementally and run the binary directly:
  ```sh
  # Fast incremental build
  cmake --build build -j$(nproc)

  # Run unit tests
  ctest --test-dir build --output-on-failure

  # Run the newly built binary directly
  ./build/DarkMark
  ```
* **For final system installation:**
  ```sh
  cd build
  make -j$(nproc) package
  sudo dpkg -i darkmark*.deb
  ```

## ONNX Runtime C++ Dependency

To enable auto-annotation with ONNX models, you need the ONNX Runtime C++ library.

**Manual Setup:**

1. Download the latest ONNX Runtime C++ package for your platform from:
   https://github.com/microsoft/onnxruntime/releases

   - For Linux: download the `onnxruntime-linux-x64-<version>.tgz`
   - For GPU support (Linux with CUDA): download `onnxruntime-linux-x64-gpu-<version>.tgz`
   - For Windows: download the `onnxruntime-win-x64-<version>.zip`
   - For GPU support (Windows with CUDA): download `onnxruntime-win-x64-gpu-<version>.zip`

   DarkMark will automatically detect and use the CUDA execution provider for GPU acceleration if you install a GPU-enabled version of ONNX Runtime.

**Important:** For GPU support, you need both CUDA and cuDNN installed on your system:
- **CUDA**: Install from [NVIDIA's website](https://developer.nvidia.com/cuda-downloads)
- **cuDNN**: Download from [NVIDIA's cuDNN page](https://developer.nvidia.com/cudnn) (requires free NVIDIA account)

**Compatibility:** Refer to the [ONNX Runtime CUDA Execution Provider requirements](https://onnxruntime.ai/docs/execution-providers/CUDA-ExecutionProvider.html#requirements) for detailed compatibility information between ONNX Runtime versions, CUDA versions, and cuDNN versions.

2. Extract the archive to `/usr/local/onnxruntime` (or `/usr/onnxruntime`), so you have:
   ```
   /usr/local/onnxruntime/lib/
   /usr/local/onnxruntime/include/
   ```
   For example:
   ```sh
   sudo tar -xzf onnxruntime-linux-x64-<version>.tgz -C /usr/local
   sudo mv /usr/local/onnxruntime-linux-x64-<version> /usr/local/onnxruntime
   sudo ldconfig
   ```

4. CMake will automatically detect and use the system-installed ONNX Runtime.

**Note:**
- CMake will not auto-download ONNX Runtime. You must perform the above steps before configuring the project.
- If the ONNX Runtime library is not found, CMake will stop with an error.
- If you encounter CUDA-related errors like "libcudnn.so.9: cannot open shared object file", ensure both CUDA and cuDNN are properly installed and the libraries are in your system's library path.

**Quick install example for ONNX Runtime 1.22.0 on Linux:**
```sh
wget https://github.com/microsoft/onnxruntime/releases/download/v1.22.0/onnxruntime-linux-x64-1.22.0.tgz
sudo tar -xzf onnxruntime-linux-x64-1.22.0.tgz -C /usr/local
sudo mv /usr/local/onnxruntime-linux-x64-1.22.0 /usr/local/onnxruntime
sudo ldconfig
```

**Note:**
If you install ONNX Runtime to `/usr/local/onnxruntime`, you must add its `lib` directory to the system library path so the dynamic linker can find `libonnxruntime.so.1` at runtime. To do this, run:

```sh
echo "/usr/local/onnxruntime/lib" | sudo tee /etc/ld.so.conf.d/onnxruntime.conf
sudo ldconfig
```

This only needs to be done once after installing or updating ONNX Runtime.

# Doxygen Output

The official DarkMark documentation and web site is at <https://www.ccoderun.ca/darkmark/>.

Some links to specific useful pages:

- [DarkMark keyboard shortcuts](https://www.ccoderun.ca/darkmark/Keyboard.html)
- ["How To" on image markup](https://www.ccoderun.ca/darkmark/ImageMarkup.html)
- [Data augmentation in Darknet](https://www.ccoderun.ca/darkmark/DataAugmentation.html)
- [Darknet configuration files](https://www.ccoderun.ca/darkmark/Configuration.html)
- [Darknet FAQ](https://www.ccoderun.ca/programming/darknet_faq/)
- [Discord server for Darknet, YOLO, DarkHelp, and DarkMark](https://discord.gg/MQw32W9Cqr)

# Developer Guidelines: Safe UI & Dialog Architecture

To prevent X11 server lockups, display server freezes, and remote desktop (e.g. RustDesk, VNC) pointer grab deadlocks, follow these architectural rules for all JUCE windows:

1. **Never call `canvas.setBounds(...)` inside `DocumentWindow::resized()`**:
   - JUCE's `DocumentWindow::resized()` automatically sets the content component (`canvas`) bounds to the interior dimensions (accounting for borders and title bars).
   - Manually resizing `canvas` inside `resized()` triggers JUCE's `childBoundsChanged`, triggering another window resize in an **infinite synchronous recursion loop** that locks the X11 server and pegs CPU at 100%.
   - Always position child components within `canvas.getLocalBounds()`.

2. **Always pass `false` to `setContentNonOwned`**:
   - Use `setContentNonOwned(&canvas, false);` to disable `resizeToFitWhenContentChangesSize`.

3. **Use modeless fake-modal instead of `runModalLoop()`**:
   - Secondary windows should be owned as `std::unique_ptr` in `DarkMarkApplication`.
   - On opening, disable the parent window (`parent->setEnabled(false);`) and display modelessly (`setVisible(true); toFront(true);`).
   - On closing, re-enable the parent window (`parent->setEnabled(true);`) and schedule cleanup asynchronously (`MessageManager::callAsync(...)`).
   - Avoid synchronous `runModalLoop()` on secondary windows, as it traps pointer events and breaks under remote display injection.

4. **Automated Enforcement**:
   - Regression tests in `src-test/TestCodebaseInvariants.cpp` run automatically with `ctest` to guarantee no `canvas.setBounds` inside `DocumentWindow::resized()` or unsafe modal loops can be committed.

