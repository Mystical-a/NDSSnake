#include <stdio.h>
#include <stdlib.h>
#include <nds.h>
#include <filesystem.h>
#include <nf_lib.h>
#include <gl2d.h>
#include <fat.h>
#include <time.h>

/* BUGS & TODO:

- Sound

*/

void initialise_bg(void);
void initialise_sprite(int);
void move(int *, int *, int *, int *, int *, int *, int *, int *);
void moveTick(int, int *, int *, int *, double);
void move_sprites(int, int *, int *);
void death(int *, int *, int *);
void checkCollisions(int *, int *, int, int *, int *);
void initialise_apple(int *, int *);
int randomiseApple(int *, int *, int *, int *, int *, int);
void appleCollisions(int **, int **, int *, int *, int **, int, int, int, int *, int *);
void increaseLength(int **, int **, int **, int *, int, int, int);

/* Constant definitions */
#define GRIDOFFSET 19
#define TICKTHRESHHOLD 15
#define GRIDXBOUND 256/GRIDOFFSET
#define GRIDYBOUND 192/GRIDOFFSET
#define OFFSETX -18
#define OFFSETY -22
#define APPLEROTOR 70
#define APPLEOFFSETX -3
#define APPLEOFFSETY -6

int main(int argc, char **argv) {

    /* Define clock cycle */
    int tick = 0;

    /* Note that the score is the length of the snake minus the starting length 2 */
    /* Text variables for the menus */
    char Score[32];
    char DeathMessage[10] = "You Died!";
    char ResetMessage[12] = "Play Again?";

    /* Variable to tell the state of the game. 0 = playing, 1 = dead, 2 = title screen */
    int menu = 0;

    /* Move offset for the animation of moving between tiles */
    double moveOffset = 0.0;

    /* ---- SPRITE TRAVEL ----
    Speed = 0.7 px a frame ( 21 / 30 ) - average
    Speed = GRID / (TICKRATE / 2)
    */
    double spriteSpeed = GRIDOFFSET / (double) (TICKTHRESHHOLD);

    /* Create counter */
    int i;

    int length=2; /* Length of snake, also length of malloc arrays */

    int *snakex = (int *) malloc(length*sizeof(int)); /* Array containing snake x values */
    int *snakey = (int *) malloc(length*sizeof(int)); /* Array containing snake y values */

    /* Direction, 0 is north, 1 is right, and so on.. */
    int *snaked = (int *) malloc((length + 1)*sizeof(int)); /* Array containing the direction of a snake sprite */

    /* Previous direction also stored to stop snake from going back on it self */
    int prevDirection = 1;

    /* Initialise snakex so it is known when to display the snake after spawning look at move() */
    /* Also initialise all the directions to be the same */
    for (i=0; i<length; i++) {
        *(snakex + i) = -10;
    }
    for (i=0; i<(length+1); i++) {
        *(snaked + i) = 1;
    }

    /* These store the grid of the last tail peice, after it has moved. Essentially where it was before */
    int lastx = 0, lasty = 0, lastd = 0;

    /* Snake starts off in the center */
    *snakex = (256 / GRIDOFFSET) / 2;
    *snakey = (192 / GRIDOFFSET) / 2;

    /* Creating Apple Sprite ID 0, ROT 0 */
    int AppleX = 0;
    int AppleY = 0;
    int AppleRot = 0;

    /* Create random engine */
    srand(time(NULL));

	/* Initialise the game objects */
	consoleDemoInit();

    initialise_bg();

    initialise_sprite(length);

    /* Randomise the applies location and reset its rotation */
    initialise_apple(&AppleX, &AppleY);
    randomiseApple(&AppleX, &AppleY, &AppleRot, snakex, snakey, length);

    /* Main loop */
    while(1) {
        /* Calculate and increment tick */
        tick++;

        /* Read keys pressed down */
        scanKeys();
        u16 keys = keysDown();

        /* Read the touchscreen */
        touchPosition touchscreen;
        touchRead(&touchscreen);

        /* ------For the game:------ */
        if (menu == 0) {

        /* Apply key presses to the snakes direction */
        if ((keys & KEY_UP) && prevDirection != 2) {

            *snaked = 0;

        } else if ((keys & KEY_DOWN) && prevDirection != 0) {

            *snaked = 2;

        } else if ((keys & KEY_LEFT) && prevDirection != 1) {

            *snaked = 3;

        } else if ((keys & KEY_RIGHT) && prevDirection != 3) {

            *snaked = 1;

        }

        /* Apply the sprites speed for the animation */
        moveOffset -= spriteSpeed;

        /* Apply the actual move onto the grid */
        moveTick(length, snakex, snakey, snaked, moveOffset);

        /* Reset tick if needed */
        if (tick == TICKTHRESHHOLD) {
            tick = 0;
            /* At 60 fps, this is called once a second */

            /* Move all the sprites to the next grid cell */
            move(&length, snakex, snakey, &lastx, &lasty, snaked, &menu, &lastd);

            /* Check for collisions and see if the snake has died */
            checkCollisions(snakex, snakey, length, snaked, &menu);
            appleCollisions(&snakex, &snakey, &AppleX, &AppleY, &snaked, lastx, lasty, lastd, &length, &AppleRot);

            /* Check if the player has died */
            if (menu == 1) {
                /* Free heap with death */
                death(snakex, snakey, snaked);

                /* Change what is displayed on screen 1 */
                NF_HideBg(1, 3);
                NF_ShowBg(1, 2);

                /* Skip to the next main loop iteration */
                continue;
            }

            /* After move is done, update prevDirection */
            prevDirection = *snaked;

            /* Reset the animation */
            moveOffset = GRIDOFFSET;
        }

        /* ------End of game segment------ */
        } else if (menu == 1) {
        /* ------Dead segment------ */

        /* Format the text to be printed */
        snprintf(Score, sizeof(Score), "You Scored: %d", (length-2));

        /* Write the text */
        NF_WriteText(1, 0, 11, 5, DeathMessage);
        NF_WriteText(1, 0, 8, 8, Score);
        NF_WriteText(1, 0, 10, 18, ResetMessage);

        /* CODE TO DISPLAY IF SCORE IS A HIGH SCORE OR NOT HERE */

        /* Check if reset button has been pressed */
        if (touchscreen.px >= 49 && touchscreen.px <= 209) {
            if (touchscreen.py >= 133 && touchscreen.py <= 169) {
                /* Play again button pressed, reset variables */

                /* Hide all of the previous sprites, they will slowly be reused and moved */
                for (i=0; i<length; i++) {
                    NF_ShowSprite(0, i+1, false);
                }

                /* Resetting variables */
                length = 2;
                tick = 0;
                menu = 0;
                moveOffset = 0.0;

                /* malloc arrays have been freed, they need to be re-made */
                snakex = (int *) malloc(length*sizeof(int)); /* Array containing snake x values */
                snakey = (int *) malloc(length*sizeof(int)); /* Array containing snake y values */
                snaked = (int *) malloc((length + 1)*sizeof(int)); /* Array containing the direction of a snake sprite */

                prevDirection = 1;

                /* Initialise snakex so it is known when to display the snake after spawning look at move() */
                /* Also initialise all the directions to be the same */
                for (i=0; i<length; i++) {
                    *(snakex + i) = -10;
                }
                for (i=0; i<(length+1); i++) {
                    *(snaked + i) = 1;
                }

                /* These store the grid of the last tail peice, after it has moved. Essentially where it was before */
                lastx = 0;
                lasty = 0;
                lastd = 0;

                /* Snake starts off in the center */
                *snakex = (256 / GRIDOFFSET) / 2;
                *snakey = (192 / GRIDOFFSET) / 2;

                /* Randomise the applies location and reset its rotation */
                randomiseApple(&AppleX, &AppleY, &AppleRot, snakex, snakey, length);

                /* Switch what background is being displayed on screen 1 */
                NF_HideBg(1, 2);
                NF_ShowBg(1, 3);

            }
        }

        /* ------End of dead segment------ */
        }

        /* Update the VRAM with text */
        NF_UpdateTextLayers();

        /* Commit to actual memory */
        NF_SpriteOamSet(0);
        NF_SpriteOamSet(1);

        /* Copy libnds OAM into the actual OAM. Must be done during VBlank */
        swiWaitForVBlank();
        oamUpdate(&oamMain);
        oamUpdate(&oamSub);

    }

	return 0;
}

/* Function to randomise the apples location and reset its rotation */
int randomiseApple(int *x, int *y, int *r, int *sx, int *sy, int L)
{
    int i, ret = 0;

    /* Create a random location on the grid */
    *x = rand() % GRIDXBOUND;
    *y = rand() % GRIDYBOUND;

    /* Reset rotation */
    *r = 0;

    /* Check if the chosen value is inside of the snake */
    for (i=0; i<L; i++) {
        if (*x == *(sx + i) && *y == *(sy + i)) {
            ret = randomiseApple(x, y, r, sx, sy, L);
        }
    }

    /* If ret is too big, then stop! 130 is maximum amount of possible spaces */
    if (ret >= 130) {
        exit(1);
    }

    /* ret is a recursion break */
    /* If an empty space has been found, move the sprites and return 1 so that it is known */
    if (!ret) {
        /* Apply changes, accounting for the offsets */
        NF_MoveSprite(0, 0, ((*x) * GRIDOFFSET + APPLEOFFSETX), ((*y) * GRIDOFFSET + APPLEOFFSETY));
        NF_SpriteRotScale(0, 0, *r, 96, 96);
        return 1;

    /* If it failed, then return the ret + 1 */
    } else {
        return (ret + 1);
    }

    /* This means that ret is the number of iterations */
}

/* Function to create the graphics for the apple */
void initialise_apple(int *x, int *y)
{
    /* (Ram slot, Pal slot, VRam slot, VPal slot) */
    /* Snake sprite in slot 0, 0, 0, 0 */
    NF_LoadSpriteGfx("Apple", 2, 32, 32);
    NF_LoadSpritePal("Apple", 2);

    /* Move to VRAM */
    NF_VramSpriteGfx(0, 2, 2, false);
    NF_VramSpritePal(0, 2, 2); /* Pal 0/15 */

    /* Create scaling pattern */
    NF_SpriteRotScale(0, 0, 0, 96, 96);

    /* Create sprite */
    NF_CreateSprite(0, 0, 2, 2, *x, *y);

    /* Apply the rotation and scaling */
    NF_EnableSpriteRotScale(0, 0, 0, false);
}

/* Check if the snake has found an apple */
void appleCollisions(int **sx, int **sy, int *x, int *y, int **sd, int lx, int ly, int ld, int *L, int *r)
{
    if (**sx != -10 && **sx == *x && **sy == *y) {
        increaseLength(sx, sy, sd, L, lx, ly, ld);

        randomiseApple(x, y, r, *sx, *sy, *L);

    }
}

/* Function to check if two snake elements are at the same location */
void checkCollisions(int *sx, int *sy, int L, int *sd, int *menu) {

    /* For all snakes in the array*/
    int i, j;

    for(i=0; i<L; i++) {

        /* Check against every other snake */
        for(j=0; j<L; j++) {

            /* Do they have the same coordinates? */
            if (*(sx + i) == *(sx + j) && i != j && *(sx + j) != -10) {
            if (*(sy + i) == *(sy + j) && i != j) {

                /* Then the snake has hit itself! */
                *menu = 1;

            }
            }
        }
    }
}

/* Function to set up the bgs */
void initialise_bg(void) {

    char *NDSPATH = "NDSSnake.nds";

    /* Initialise console */
    consoleDemoInit();

    NF_Set2D(0, 0);
    NF_Set2D(1, 0);

    /*fatInitDefault();*/
    nitroFSInit(&NDSPATH);

    /* Initialise file system */
    NF_SetRootFolder("NITROFS");

    /* Start buffers */
    NF_InitTiledBgBuffers();
    NF_InitTiledBgSys(0);
    NF_InitTiledBgSys(1);

    /* Load the game backgrounds */
    NF_LoadTiledBg("SnakeGround", "SnakeGround", 256, 256);
    NF_LoadTiledBg("starField", "starField", 256, 256);

    NF_CreateTiledBg(0, 3, "SnakeGround");
    NF_CreateTiledBg(1, 3, "starField");

    /* Load the menu backgrounds (layer 2) */
    NF_LoadTiledBg("MenuGround", "MenuGround", 256, 256);
    NF_CreateTiledBg(1, 2, "MenuGround");
    NF_HideBg(1, 2);

    /* Initialise the text engine to display info on screen 1 */
    NF_InitTextSys(1);

    /* Load the font */
    NF_LoadTextFont("font", "normal", 256, 256, 0);
    NF_CreateTextLayer(1, 0, 0, "normal");

    /* Set the texts colour */
    NF_DefineTextColor(1, 0, 0, 0, 0, 0);
    NF_SetTextColor(1, 0, 0);

    /* Update the text */
    NF_UpdateTextLayers();

}

void initialise_sprite(int L)
{
    int i;

    /* Initialise sprite buffers */
    NF_InitSpriteBuffers();
    NF_InitSpriteSys(0, 128);
    NF_InitSpriteSys(1, 64);

    /* (Ram Gfx slot, Pal slot, VRam slot, VPal slot) */
    /* Snake sprite in slot 1, 1, 1, 1 */
    NF_LoadSpriteGfx("Snake", 1, 32, 32);
    NF_LoadSpritePal("Snake", 1);

    /* Move to VRAM */
    NF_VramSpriteGfx(0, 1, 1, false);
    NF_VramSpritePal(0, 1, 1); /* Pal 1/15 */

    /* Create scaling pattern */
    /* To work with 21x21 grid, scale 376 and offset 4 */
    NF_SpriteRotScale(0, 1, 0, 96, 96);

    for (i=1; i<=L; i++) {

        /* Create the snake, put the sprites out of frame */
        NF_CreateSprite(0, i, 1, 1, 0, 0);

        /* Hide them until they have been moved */
        NF_ShowSprite(0, i, false);

        /* Apply Scaling */
        NF_EnableSpriteRotScale(0, i, 1, true);
    }
}

/* Function to increase the length of the snake */
void increaseLength(int **sx, int **sy, int **sd, int *L, int lx, int ly, int ld)
{
    /* Increment L */
    *L = *L + 1;

    /* Reallocate sx, sy, sd, check for memory errors to exit safely */
    *sx = (int *) realloc((void *) *sx, (*L) * sizeof(int));
    *sy = (int *) realloc((void *) *sy, (*L) * sizeof(int));
    *sd = (int *) realloc((void *) *sd, (*L + 1) * sizeof(int));

    /* Check that the arrays are not NULL */
    if (!(*sx) || !(*sy) || !(*sd)) {
        iprintf("Memory error when increasing length\n");
    }

    /* Insert the correct values into the last element of the array */
    *(*sx + *L - 1) = lx;
    *(*sy + *L - 1) = ly;
    *(*sd + *L) = ld;

    /* Create another sprite, place it off screen */
    NF_CreateSprite(0, *L, 1, 1, 0, 200);
    NF_EnableSpriteRotScale(0, *L, 1, true);

}

/* Function called when the snake dies */
void death(int *sx, int *sy, int *sd)
{
    /* Free used heap memory */
    free(sx);
    free(sy);
    free(sd);
}

void moveTick(int L, int *sx, int *sy, int *sd, double moveOffset)
{
    int i, x, y;

    for(i=0; i<L; i++) {

        x = *(sx+i) * GRIDOFFSET + OFFSETX;
        y = *(sy+i) * GRIDOFFSET + OFFSETY;

        switch (*(sd + i + 1)) {

        /* Sprite moving up */
        case 0:
            y += (int) moveOffset;
            break;

        /* Sprite moving right */
        case 1:
            x -= (int) moveOffset;
            break;

        /* Sprite moving down */
        case 2:
            y -= (int) moveOffset;
            break;

        /* Sprite moving left */
        case 3:
            x += (int) moveOffset;
            break;
        }

        /* calculate the move */
        NF_MoveSprite(0, (i+1), x, y);
    }}

void move(int *L, int *sx, int *sy, int *lx, int *ly, int *sd, int *dead, int *ld)
{
    int i;

    /* Save the last element */
    *lx = *(sx+*L-1);
    *ly = *(sy+*L-1);
    *ld = *(sd+*L);

    /* Shift the rest down the array by one place */
    for (i=(*L)-1; i > 0; i--) {
        *(sx + i) = *(sx + i - 1);
        *(sy + i) = *(sy + i - 1);
    }

    /* Shift the directions down */
    for (i=(*L); i > 0; i--) {
        /* If there is a change in direction, then there is a corner at that location */
        *(sd + i) = *(sd + i - 1);
    }

    /* Check all of the sprites */
    for (i=1; i<=(*L); i++) {
        /* If the sprites coordinates exist, show it */
        if (*(sx + i - 1) != -10)
        NF_ShowSprite(0, i, true);
    }

    /* Filling in the first element of the list */
    switch (*sd) {

    case 0:
        *sy -= 1;
        break;
    case 1:
        *sx += 1;
        break;
    case 2:
        *sy += 1;
        break;
    case 3:
        *sx -= 1;
        break;
    }

    /* Validating move, if out of range, loop back */

    /* For all of the parts of the snake */
    for (i=0; i<(*L); i++) {
        /* If the snakes x is -10, skip this */
        if (*(sx + i) == -10) {
            continue;
        }

        /* X bounds */
        if (*(sx + i) > (GRIDXBOUND)) {
            *(sx + i) = -1;
        }
        if (*(sx + i) < -1) {
            *(sx + i) = GRIDXBOUND;
        }

        /* Y bounds */
        if (*(sy + i) > (GRIDYBOUND)) {
            *(sy + i) = -1;
        }
        if (*(sy + i) < -1) {
            *(sy + i) = GRIDYBOUND;
        }
    }
}

/* Clips sprites back onto grid after animation */
/* FUNCTION NO LONGER USED -------------------- */
void move_sprites(int L, int *sx, int *sy)
{
    int i, x, y;

    /* For all the sprites */
    for (i=0; i<(L); i++) {
        /* Put in place */
        x = *(sx+i) * GRIDOFFSET + OFFSETX;
        y = *(sy+i) * GRIDOFFSET + OFFSETY;

        /* Move the sprite */
        NF_MoveSprite(0, (i+1), x, y);

    }
}
