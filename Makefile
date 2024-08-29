# Variables
CC = gcc
CFLAGS = -Wall -std=c99 -lm -lrt -pthread -g -D_XOPEN_SOURCE=500
EXECUTABLES = slave md5 view
OBJECTS = slave.o md5.o view.o shm_manager.o

# Regla principal para construir ambos ejecutables
all: $(EXECUTABLES)
	@echo "Build complete. Cleaning up object files..."
	$(MAKE) clean_objects

# Regla para construir el ejecutable slave
slave: slave.o 
	$(CC) $(CFLAGS) -o $@ $^

# Regla para construir el ejecutable md5
md5: md5.o  shm_manager.o
	$(CC) $(CFLAGS) -o $@ $^

# Regla para construir el ejecutable view
view: view.o shm_manager.o
	$(CC) $(CFLAGS) -o $@ $^

# Regla para construir el archivo objeto slave.o desde slave.c
slave.o: slave.c 
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para construir el archivo objeto md5.o desde md5.c
md5.o: md5.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para construir el archivo objeto view.o desde view.c
view.o: view.c 
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para construir el archivo objeto shm_manager.o desde shm_manager.c
shm_manager.o: shm_manager.c 
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para construir el archivo objeto pipe_master.o desde pipe_master.c
pipe_master.o: pipe_master.c 
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para eliminar los archivos objeto
clean_objects:
	rm -f $(OBJECTS)

# Regla para limpiar todo (ejecutables y otros archivos generados)
clean:
	rm -f $(EXECUTABLES) salida.txt PVS-Studio.log report.tasks strace_out
	rm -rf .config
	$(MAKE) clean_objects

.PHONY: all clean clean_objects
