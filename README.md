# Sparse Check

The code checks whether a file is a sparse file or not. If the file is sparse, the fiemap structure is used to retrieve extents and a new file is generated which does not have any sparse blocks. The gistfile1.txt is originally a 1MB file which is truncated to 5MB. Using this newFile.txt is generated.

## Usage

Inside the repository simply run

```bash
gcc checkSparse.c
./a.out

```