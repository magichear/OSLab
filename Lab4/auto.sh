# 下载源码
cd ~/oslab
wget -O oh4.0_mini.tar.zst https://git.ustc.edu.cn/ldeng/ustc-os-resources/-/raw/main/oh4.0_mini_no_kernel_20240423.tar.zst

echo "3243ee90de1eead40a70156a47992c07d657f5a7f739a955820a37a0e2370a5f oh4.0_mini.tar.zst" | sha256sum --check
# 或者下载完整版
wget -O oh4.0_mini.tar.zst https://git.ustc.edu.cn/ldeng/ustc-os-resources/-/raw/main/oh4.0_mini_20240423.tar.zst
echo "43808b4a13afc285627437ab69178ef92cfd72957f1a8d75ad3bf3d8e70c36ee oh4.0_mini.tar.zst" | sha256sum --check

echo plz check if any problem
echo Enter to continue
echo Ctrl and C to quit
read dummy_variable0

unzstd -c oh4.0_mini.tar.zst | tar xvf -

 # 下载预编译内容
wget https://git.ustc.edu.cn/ldeng/ustc-os-resources/-/raw/main/oh4.0_mini_prebuilts_20240423.tar.gz

echo "957af04fab034d646c4a20aaf2e35f46ce96b219d5f8d824373907374ae38b21 oh4.0_mini_prebuilts_20240423.tar.gz" | sha256sum --check

echo plz check if any problem
echo Enter to continue
echo Ctrl and C to quit
read dummy_variable1

mkdir OpenHarmony-v4.0-Release/openharmony_prebuilts

tar xvf oh4.0_mini_prebuilts_20240423.tar.gz --directory=OpenHarmony-v4.0-Release/openharmony_prebuilts

cd OpenHarmony-v4.0-Release/OpenHarmony

./build/prebuilts_download.sh

# 安装miniconda
# wget https://mirrors.bfsu.edu.cn/anaconda/miniconda/Miniconda3-latest-Linux-x86_64.sh
# 
# bash Miniconda3-latest-Linux-x86_64.sh -b
# 
# ~/miniconda3/bin/conda init
# 
# source ~/.bashrc
# 
# # 建立python环境
# conda create -y --name oh python=3.8 --no-default-packages
# conda activate oh   # 激活对应环境
# python -V
# 
# echo 请将以下两行添加至 ~/.bashrc 的末尾
# echo conda activate oh
# echo export PATH=~/.local/bin:$PATH
# read dummy_variable2


cd ~/oslab/OpenHarmony-v4.0-Release/OpenHarmony
python3 -m pip install --user build/hb

hb help

echo plz check if any problem
echo Enter to continue
echo Ctrl and C to quit
read dummy_variable3

sudo apt update
sudo apt install build-essential flex bison ruby libgl1-mesa-dev libglib2.0-dev libncurses5-dev libxcursor-dev libxrandr-dev libxinerama-dev libexpat1-dev default-jdk libelf-dev ccache libssl-dev genext2fs liblz4-tool curl libstdc++-11-dev libstdc++-12-dev


# 编译
hb set -p rk3568_mini_system ; hb build --gn-args load_test_config=False --ccache=False