all: lemmings.exe

lemmings.exe: main.obj game.obj render.obj dat.obj level.obj sprite.obj
	link /nologo /subsystem:windows main.obj game.obj render.obj dat.obj level.obj sprite.obj user32.lib gdi32.lib kernel32.lib /out:lemmings.exe

main.obj: main.c game.h render.h
	cl /nologo /O2 /W3 /c main.c

game.obj: game.c game.h render.h level.h sprite.h
	cl /nologo /O2 /W3 /c game.c

render.obj: render.c render.h game.h
	cl /nologo /O2 /W3 /c render.c

dat.obj: dat.c dat.h
	cl /nologo /O2 /W3 /c dat.c

level.obj: level.c level.h dat.h game.h
	cl /nologo /O2 /W3 /c level.c

sprite.obj: sprite.c sprite.h dat.h game.h
	cl /nologo /O2 /W3 /c sprite.c

clean:
	del *.obj lemmings.exe

