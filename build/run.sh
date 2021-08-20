###
 # @Author: Cai Deng
 # @Date: 2021-03-06 06:45:48
 # @LastEditors: Cai Deng
 # @LastEditTime: 2021-07-17 03:00:53
 # @Description: 
### 
window_size=(1 2 4 8)
feature_num=(2 4 6 8 10 12)
dataset=(wmvoc wminet taobao/tb jingdong)

###
 # window size :
###

# for ((i=0; i<${#dataset[@]}; i++))
# do 
#     for ((j=0; j<${#window_size[@]}; j++))
#     do
#         sudo sync
#         sudo echo 3 > /proc/sys/vm/drop_caches
#         sudo rm -r /home/dc/data/image_data/dst/wm/*
#         sudo /home/dc/image_dedup/iDedup/build/sid -c --input_path=/home/dc/data/image_data/${dataset[$i]}/ \
#         --output_path=/home/dc/data/image_data/dst/wm/ --read_thrd_num=1 --decode_thrd_num=8 --middle_thrd_num=1 \
#         --write_thrd_num=1 --buffer_size=G64 --patch_size=G1 --name_list=G1 --read_list=G2 --indx_list=G2 \
#         --decd_list=G2 --dect_list=G2 --deup_list=G2 --rejg_list=G2 --chunking=variable \
#         --road_num=1 --chaos=no --sf_num=10 --sf_component_num=1 --feature_method=2df \
#         --block_size=${window_size[$j]} --dimension=2 --delta=idelta --data_type=decoded
#     done
# done

###
 # feature number :
###

feature_num=(10)

# for ((i=0; i<${#dataset[@]}; i++))
# do 
#     for ((j=0; j<${#feature_num[@]}; j++))
#     do
#         sudo sync
#         sudo echo 3 > /proc/sys/vm/drop_caches
#         sudo rm -r /home/dc/data/image_data/dst/wm/*
#         sudo /home/dc/image_dedup/iDedup/build/sid -c --input_path=/home/dc/data/image_data/${dataset[$i]}/ \
#         --output_path=/home/dc/data/image_data/dst/wm/ --read_thrd_num=1 --decode_thrd_num=4 --middle_thrd_num=1 \
#         --write_thrd_num=1 --buffer_size=G64 --patch_size=G1 --name_list=G1 --read_list=G2 --indx_list=G2 \
#         --decd_list=G2 --dect_list=G2 --deup_list=G2 --rejg_list=G2 --chunking=variable \
#         --road_num=1 --chaos=no --sf_num=${feature_num[$j]} --sf_component_num=1 --feature_method=2df \
#         --block_size=2 --dimension=2 --delta=idelta --data_type=decoded
#     done
# done



# pro_name=(sid-none)

# for ((i=0; i<${#dataset[@]}; i++))
# do 
#     for ((j=0; j<${#pro_name[@]}; j++))
#     do
#         sudo sync
#         sudo echo 3 > /proc/sys/vm/drop_caches
#         sudo rm -r /home/dc/data/image_data/dst/wm/*
#         sudo /home/dc/image_dedup/iDedup/build/${pro_name[$j]} -c --input_path=/home/dc/data/image_data/${dataset[$i]}/ \
#         --output_path=/home/dc/data/image_data/dst/wm/ --read_thrd_num=1 --decode_thrd_num=8 --middle_thrd_num=1 \
#         --write_thrd_num=1 --buffer_size=G64 --patch_size=G1 --name_list=G1 --read_list=G2 --indx_list=G2 \
#         --decd_list=G2 --dect_list=G2 --deup_list=G2 --rejg_list=G2 --chunking=variable \
#         --road_num=1 --chaos=no --sf_num=10 --sf_component_num=1 --feature_method=2df \
#         --block_size=2 --dimension=2 --delta=idelta --data_type=decoded
#     done
# done


pro_name=(sid-fse)

# for ((i=0; i<${#dataset[@]}; i++))
# do 
#     for ((j=0; j<${#pro_name[@]}; j++))
#     do
#         sudo sync
#         sudo echo 3 > /proc/sys/vm/drop_caches
#         sudo rm -r /home/dc/data/image_data/dst/wm/*
#         sudo /home/dc/image_dedup/iDedup/build/${pro_name[$j]} -c --input_path=/home/dc/data/image_data/${dataset[$i]}/ \
#         --output_path=/home/dc/data/image_data/dst/wm/ --read_thrd_num=1 --decode_thrd_num=8 --middle_thrd_num=1 \
#         --write_thrd_num=1 --buffer_size=G64 --patch_size=G1 --name_list=G1 --read_list=G2 --indx_list=G2 \
#         --decd_list=G2 --dect_list=G2 --deup_list=G2 --rejg_list=G2 --chunking=variable \
#         --road_num=1 --chaos=no --sf_num=10 --sf_component_num=1 --feature_method=2df \
#         --block_size=2 --dimension=2 --delta=xdelta --data_type=decoded
#     done
# done



###
 # 1-d byte-wise :
###

window_size=(64 128)

# for ((i=0; i<${#dataset[@]}; i++))
# do 
#     for ((j=0; j<${#window_size[@]}; j++))
#     do
#         sudo sync
#         sudo echo 3 > /proc/sys/vm/drop_caches
#         sudo rm -r /home/dc/data/image_data/dst/wm/*
#         sudo /home/dc/image_dedup/iDedup/build/sid -c --input_path=/home/dc/data/image_data/${dataset[$i]}/ \
#         --output_path=/home/dc/data/image_data/dst/wm/ --read_thrd_num=4 --decode_thrd_num=8 --middle_thrd_num=4 \
#         --write_thrd_num=4 --buffer_size=G64 --patch_size=G1 --name_list=G1 --read_list=G2 --indx_list=G2 \
#         --decd_list=G2 --dect_list=G2 --deup_list=G2 --rejg_list=G2 --chunking=variable \
#         --road_num=1 --chaos=no --sf_num=10 --sf_component_num=1 --feature_method=rabin \
#         --block_size=${window_size[$j]} --dimension=1 --delta=idelta --data_type=decoded
#     done
# done

###
 # 1-d block-wise :
###

window_size=(1)

# for ((i=0; i<${#dataset[@]}; i++))
# do 
#     for ((j=0; j<${#window_size[@]}; j++))
#     do
#         sudo sync
#         sudo echo 3 > /proc/sys/vm/drop_caches
#         sudo rm -r /home/dc/data/image_data/dst/wm/*
#         sudo /home/dc/image_dedup/iDedup/build/sid -c --input_path=/home/dc/data/image_data/${dataset[$i]}/ \
#         --output_path=/home/dc/data/image_data/dst/wm/ --read_thrd_num=1 --decode_thrd_num=8 --middle_thrd_num=1 \
#         --write_thrd_num=1 --buffer_size=G64 --patch_size=G1 --name_list=G1 --read_list=G2 --indx_list=G2 \
#         --decd_list=G2 --dect_list=G2 --deup_list=G2 --rejg_list=G2 --chunking=variable \
#         --road_num=1 --chaos=no --sf_num=10 --sf_component_num=1 --feature_method=rabin \
#         --block_size=${window_size[$j]} --dimension=1.5 --delta=idelta --data_type=decoded
#     done
# done

###
 # 2-d block-wise :
###

window_size=(2 4)

# for ((i=0; i<${#dataset[@]}; i++))
# do 
#     for ((j=0; j<${#window_size[@]}; j++))
#     do
#         sudo sync
#         sudo echo 3 > /proc/sys/vm/drop_caches
#         sudo rm -r /home/dc/data/image_data/dst/wm/*
#         sudo /home/dc/image_dedup/iDedup/build/sid -c --input_path=/home/dc/data/image_data/${dataset[$i]}/ \
#         --output_path=/home/dc/data/image_data/dst/wm/ --read_thrd_num=4 --decode_thrd_num=8 --middle_thrd_num=4 \
#         --write_thrd_num=4 --buffer_size=G64 --patch_size=G1 --name_list=G1 --read_list=G2 --indx_list=G2 \
#         --decd_list=G2 --dect_list=G2 --deup_list=G2 --rejg_list=G2 --chunking=variable \
#         --road_num=1 --chaos=no --sf_num=10 --sf_component_num=1 --feature_method=rabin \
#         --block_size=${window_size[$j]} --dimension=2 --delta=idelta --data_type=decoded
#     done
# done


###
 # 1-d byte-wise (in rabin) :
###

window_size=(64)

# for ((i=0; i<${#dataset[@]}; i++))
# do 
#     for ((j=0; j<${#window_size[@]}; j++))
#     do
#         sudo sync
#         sudo echo 3 > /proc/sys/vm/drop_caches
#         sudo rm -r /home/dc/data/image_data/dst/wm/*
#         sudo /home/dc/image_dedup/iDedup/build/sid -c --input_path=/home/dc/data/image_data/${dataset[$i]}/ \
#         --output_path=/home/dc/data/image_data/dst/wm/ --read_thrd_num=1 --decode_thrd_num=8 --middle_thrd_num=1 \
#         --write_thrd_num=1 --buffer_size=G64 --patch_size=G1 --name_list=G1 --read_list=G2 --indx_list=G2 \
#         --decd_list=G2 --dect_list=G2 --deup_list=G2 --rejg_list=G2 --chunking=variable \
#         --road_num=1 --chaos=no --sf_num=10 --sf_component_num=1 --feature_method=rabin \
#         --block_size=${window_size[$j]} --dimension=1 --delta=idelta --data_type=decoded
#     done
# done



###
 # 1-d byte-wise (in gear odess) :
###

window_size=(64)

# for ((i=0; i<${#dataset[@]}; i++))
# do 
#     for ((j=0; j<${#window_size[@]}; j++))
#     do
#         sudo sync
#         sudo echo 3 > /proc/sys/vm/drop_caches
#         sudo rm -r /home/dc/data/image_data/dst/wm/*
#         sudo /home/dc/image_dedup/iDedup/build/sid-odess -c --input_path=/home/dc/data/image_data/${dataset[$i]}/ \
#         --output_path=/home/dc/data/image_data/dst/wm/ --read_thrd_num=1 --decode_thrd_num=8 --middle_thrd_num=1 \
#         --write_thrd_num=1 --buffer_size=G64 --patch_size=G1 --name_list=G1 --read_list=G2 --indx_list=G2 \
#         --decd_list=G2 --dect_list=G2 --deup_list=G2 --rejg_list=G2 --chunking=variable \
#         --road_num=1 --chaos=no --sf_num=10 --sf_component_num=1 --feature_method=gear \
#         --block_size=${window_size[$j]} --dimension=1 --delta=idelta --data_type=decoded
#     done
# done


dataset=(wmvoc wminet taobao/tb jingdong)
thrd_num=(3)
decd_num=(4)

for ((i=0; i<${#dataset[@]}; i++))
do 
    for ((j=0; j<${#thrd_num[@]}; j++))
    do
        sudo sync
        sudo echo 3 > /proc/sys/vm/drop_caches
        sudo rm -r /home/dc/data/image_data/dst/wm/*
        sudo /home/dc/image_dedup/iDedup/build/sid -c --input_path=/home/dc/data/image_data/${dataset[$i]}/ \
        --output_path=/home/dc/data/image_data/dst/wm/ --read_thrd_num=1 --decode_thrd_num=${decd_num[$j]} --middle_thrd_num=${thrd_num[$j]} \
        --write_thrd_num=1 --buffer_size=G64 --patch_size=G1 --name_list=G1 --read_list=G2 --indx_list=G2 \
        --decd_list=G2 --dect_list=G2 --deup_list=G2 --rejg_list=G2 --chunking=variable \
        --road_num=1 --chaos=no --sf_num=10 --sf_component_num=1 --feature_method=2df \
        --block_size=2 --dimension=2 --delta=idelta --data_type=decoded
    done
done