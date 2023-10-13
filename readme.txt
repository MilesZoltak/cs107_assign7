File: readme.txt
Author: Miles Zoltak
----------------------

implicit
--------
For implicit, I feel like there weren't really any design decisions I made except for choosing memory
blocks in mymalloc().  For that I just decided to do "first fit" because it felt the most
straightforward to me and I figured for any method a test could come around and exploit its
weaknesses so there was no point in agonizing over which method I used.

explicit
--------
The first thing I wanna say in this is that I REALLY wanted to do a BST for the free list.  I had big
dreams, but I pretty quickly realized it was not gonna be a realistic goal.  It would have been cool
though, especially if I did a self-balancing tree, finding a block would be SO fast and I would be
able to avoid fracturing the heap.  But also Nick (Troccoli) said that it wasn't recommended anyway,
maybe it's just inefficient because it's so much overhead.  The only particularly cool thing I did
was making my coalescing function (which I just called a merge function because I didn't want to
spell out coalseslces) recursive, that was kinda fun and it actually worked out pretty well.  Also,
using bitfields was super helpful, and I think it really cut down on instruction counts.

Tell us about your quarter in CS107!
-----------------------------------
You folks are probably aware this class has a bit of a reputation, all the memes and whatnot.  I went
in HOPING to pass, and so I think that made me up my game and work really hard to stay on top of
stuff.  I was very diligent about starting assignments early (sometimes the morning they came out)
and just chipping away whenever I could.  It didn't make things totally painless, but I always got
the on time bonus and rarely felt up against the wall.  The only exception was Binary Bomb, which I
had to start a few days late due to an essay.  That assignment kicked my butt, and I felt pretty much
traumatized by GDB and Assembly.  I started this assignment the day it came out and I worked on it
almost every day of the break and it payed off.  I can't stress how much staying on top of this class
made it bearable, even enjoyable.

Thanks for a wonderful experience. Nick was a great lecturer and I could tell how much teaching this
class meant to him.  Whatever the opposite of "mailing it in" is, he did that every time he came in
for a lecture.  Rifath was an awesome lab leader.  He really knew what he was talking about, he was
approachable, and he gave me some motivational speeches when I really needed it, like after getting a
D on the midterm.  This class is tough, but a lot of great things are.
