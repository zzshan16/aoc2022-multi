#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

int count_arr[16];
char* global_map;
size_t global_size;
static inline int process_fun(char* map, size_t size){
  int a, count0, count1;
  int nums[4];
  size_t offset = 0;
  a = count0 = count1 = 0;
  memset(nums,0,4*sizeof(int));
  while(offset<size){
    switch(map[offset]){
    case '-':
    case ',':
      a++;
      break;
    case '\n':
      if ((nums[0]-nums[2]) * (nums[1]-nums[3]) <= 0) count0++;
      if ((nums[0]-nums[3]) * (nums[1]-nums[2]) <= 0) count1++;
      a = 0;
      memset(nums, 0, 4*sizeof(int));
      break;
    default:
      nums[a] *= 10;
      nums[a] += map[offset] - '0';
      break;
    }
    offset++;
  }
  count_arr[0] = count0;
  count_arr[8] = count1;
  return 0;
}
void* process_multi(char* arr){
  char this_thread = **(char**)arr;
  size_t offset = **((size_t**)(arr+256));//assume pointers are same size
  size_t right_bound = **((size_t**)(arr+256 + sizeof(void*)));
  
  int a, count0, count1;
  int nums[4];
  a = count0 = count1 = 0;
  memset(nums,0,4*sizeof(int));
  
  if (offset){
    while(*(global_map+offset++) != '\n');
  }
  while(offset<right_bound){
    switch(global_map[offset]){
    case '-':
    case ',':
      a++;
      break;
    case '\n':
      if ((nums[0]-nums[2]) * (nums[1]-nums[3]) <= 0) count0++;
      if ((nums[0]-nums[3]) * (nums[1]-nums[2]) <= 0) count1++;
      a = 0;
      memset(nums, 0, 4*sizeof(int));
      break;
    default:
      nums[a] *= 10;
      nums[a] += global_map[offset] - '0';
      break;
    }
    offset++;
  }
  if (right_bound != global_size){
    do{
      switch(global_map[offset]){
      case '-':
      case ',':
	a++;
	break;
      case '\n':
	if ((nums[0]-nums[2]) * (nums[1]-nums[3]) <= 0) count0++;
	if ((nums[0]-nums[3]) * (nums[1]-nums[2]) <= 0) count1++;
	goto done;
    default:
	nums[a] *= 10;
	nums[a] += global_map[offset] - '0';
	break;
      }
      offset++;
    } while (offset <= global_size);//while(1)?
  }
 done:
  count_arr[this_thread] = count0;
  count_arr[this_thread+8] = count1;
  return 0;
}

int main(int argc, char** argv){
  struct timespec start_time, end_time;
  long signed int nsec;
  long unsigned int sec;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  struct stat statbuf;
  int fd = open("input", O_RDONLY);
  fstat(fd, &statbuf);
  size_t filesize = statbuf.st_size;
  char* input_map = (char*)mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  global_map = input_map;
  global_size = filesize;
  size_t offset_arr[9];
  if (!(filesize >> 16)){
    process_fun(input_map, filesize);
  }
  else{
    offset_arr[0] = 0;
    offset_arr[1] = filesize >> 3;
    offset_arr[2] = filesize >> 2;
    offset_arr[4] = filesize >> 1;
    offset_arr[8] = filesize;	
    offset_arr[3] = offset_arr[1] + offset_arr[2];
    offset_arr[5] = offset_arr[4] + offset_arr[1];
    offset_arr[6] = offset_arr[4] + offset_arr[2];
    offset_arr[7] = offset_arr[4] + offset_arr[3];
    char* ptrbuf = malloc(512);
    memset(ptrbuf, 0, 512);
    for(int i = 0; i < 8; ++i){
      *(ptrbuf + 500 + i) = (char)i;
      *(char**)(ptrbuf + i * sizeof(char*)) = ptrbuf+500+i;
      *(size_t**)(ptrbuf + 256 + i * sizeof(size_t*)) = offset_arr + i;
    }
    *(size_t**)(ptrbuf + 256 + 8*sizeof(size_t*)) = offset_arr + 8;
    
    pthread_t *threads = malloc(8*sizeof(pthread_t));
    for(int i = 0; i < 8; ++i){
      pthread_create(threads+i, 0, (void*(*)(void*))process_multi, ptrbuf+i*sizeof(char*));
    }
    for(int i = 0; i < 8; ++i){
      pthread_join(*(threads+i), 0);
    }    
    for(int i = 1; i < 8; ++i){
      count_arr[0] += count_arr[i];
      count_arr[8] += count_arr[8+i];
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  nsec = end_time.tv_nsec - start_time.tv_nsec;
  sec = end_time.tv_sec - start_time.tv_sec;
  if (nsec < 0){
    --sec;
    nsec += 1000000000;
  }
  munmap(input_map, filesize);
  printf("part 1 = %d, part2 = %d\ntime elapsed: %lu seconds and %ld nanoseconds\n", count_arr[0], count_arr[8], sec, nsec);
  return 0;
}
