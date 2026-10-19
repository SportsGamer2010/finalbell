# Final Bell — Undisputed PS5 controls (Competitor)

This is the default **Competitor** scheme from Undisputed (Steel City Interactive), on a PlayStation 5 pad. Face buttons and the right stick are two ways to throw the same punch. A button does not also need the stick. Southpaw reverses lead and rear on those same buttons and stick directions, which is what the Steel City tutorial says.

Sources, read before this page was written:

- Steel City Interactive, Jeremy Shepard (community manager), "Learn how to Play Undisputed": https://www.youtube.com/watch?v=5lqSvAQz-tM — Square jab, Triangle straight, X lead hook, Circle rear hook, Square+X lead uppercut, Triangle+Circle rear uppercut, the matching right-stick directions, hold L1 for the body, hold R1 for power. On the block: "You engage that by pressing R2 to put your hands up and then you push down on the left stick to put your arms down to block your body."
- GINX, Competitor scheme (they call it the default), 7 October 2024: https://www.ginx.tv/en/controls-guide-2 — "Hold Block" is Hold R2, "Hold Weave" is Hold L2. The same table has L3 slip, R3 push, the D-pad, R1+left stick diagonal dodge, L1+D-pad down low blow, and R1+D-pad down take a knee. GINX lists the right uppercut as X+Square. That repeats the left uppercut. This page follows the Steel City video: Circle+Triangle, and the right stick down-right.
- PlayStation store listing: pause works during offline play. It does not name the button. Competitor's published table does not assign the touchpad.

L1 and R1 are the shoulder buttons. L2 and R2 are the analog triggers under the index fingers, not another pair of shoulders. On a DualSense those triggers are `Gamepad_LeftTriggerAxis` (L2) and `Gamepad_RightTriggerAxis` (R2). A pull past about a third of the travel counts as held. Letting go counts as up.

**Final Bell difference, from the latest instruction.** Undisputed uses L1 and R1 as holds only (body and power). Here a short tap still rolls: L1 rolls left, R1 rolls right, even if no punch is thrown. Holding either one still changes the punch for the whole press. A tap is a press shorter than a quarter of a second that did not throw, foul, or (for R1) dodge. L2 and R2 stay the Undisputed holds: weave and block.

The capsule and the punch spheres do not follow the roll. The bend is on `pelvis`, `spine_01`, `spine_02`, `spine_03`, `neck_01`, and `head`. A side roll commands 48° through the trunk (it was 32°). The neck gives back 0.40 of that (it was 0.70), and the head keeps 0.22, so the head stays about 39° off the punch line instead of about 17°. The low shoulder drops about 17 cm on top of that roll. Lean back commands -58° (was -34°). A roll under a hook commands 74° forward and 42° to the side (was 50° and 24°). The pose holds for 0.70 seconds.

| Action | PlayStation | Xbox | Keyboard |
| --- | --- | --- | --- |
| Move / run | Left stick | Left stick | WASD |
| Jab | Square, or right stick up-left | X, or right stick up-left | J, 1, or left mouse. Arrows up-left |
| Straight | Triangle, or right stick up-right | Y, or right stick up-right | K, 2, or right mouse. Arrows up-right |
| Lead hook | X, or right stick left | A, or right stick left | U, 3, or middle mouse. Arrow left |
| Rear hook | Circle, or right stick right | B, or right stick right | I, 4, or mouse thumb. Arrow right |
| Lead uppercut | X + Square, or right stick down-left | A + X, or right stick down-left | U+J, or Y / 5. Arrows down-left |
| Rear uppercut | Circle + Triangle, or right stick down-right | B + Y, or right stick down-right | I+K, or H / 6. Arrows down-right |
| Body shot | Hold L1, then the punch | Hold LB, then the punch | Hold C, then the punch |
| Power shot | Hold R1, then the punch | Hold RB, then the punch | Hold V, then the punch |
| Body power shot | Hold L1 and R1, then the punch | Hold LB and RB, then the punch | Hold C and V, then the punch |
| Roll left | Tap L1 | Tap LB | Tap C |
| Roll right | Tap R1 | Tap RB | Tap V |
| Block, head | Right trigger (R2), held | Right trigger (RT), held | Hold Left Shift |
| Block, body | Right trigger (R2) held, left stick down | Right trigger (RT) held, left stick down | Hold Left Shift and S |
| Weave | Left trigger (L2), held. Stick forward or neutral rolls under. Stick back leans away. Stick sideways drops that shoulder | Left trigger (LT), held, same stick | Hold Left Ctrl. W rolls under. S leans back. A or D drops that side |
| Diagonal dodge | Hold R1 and move the left stick | Hold RB and move the left stick | Hold V and WASD |
| Slip | L3 while the left stick is held | LS click while the left stick is held | Left Alt with WASD |
| Loose movement | D-pad up | D-pad up | B |
| Switch stance | D-pad right | D-pad right | G |
| Taunt / feint | D-pad left | D-pad left | T |
| Headbutt | D-pad down | D-pad down | N |
| Low blow | Hold L1 and press D-pad down | Hold LB and press D-pad down | Hold C and press N |
| Take a knee | Hold R1 and press D-pad down | Hold RB and press D-pad down | Hold V and press N |
| Push | R3 | RS click | Right Alt |
| Clinch | L3 + R3 together | LS + RS click | Left Alt + Right Alt |
| Camera orbit | — | — | Mouse, or Q and E |
| Pause | Options | Menu | Escape |

Create / Share (the touchpad, `Gamepad_Special_Left`) is not a Competitor fight button in the sources above. It is not bound during a bout. Options pauses and resumes. That matches the menu that is already shipped.

A clinch is not in the current Undisputed Competitor list. Steel City's October 2024 launch notes say clinch was removed. Final Bell still has one, on L3+R3 and on Left Alt+Right Alt. R3 by itself is the push.

Upper-body defense uses those inputs. It does not add a button. A slip still steps the capsule and still grants the angle. A tap roll does not step the capsule. The server treats the pose as a head slip when it was already up. Body shots still land. The glove sphere aims at the ribs through the existing body-target intent. The client does not send a damage number.

| Pose | How to get it |
| --- | --- |
| Roll left | Tap L1 or C. Also L3 or Left Alt with the stick held left |
| Roll right | Tap R1 or V. Also L3 or Left Alt with the stick held right |
| Roll under a hook | Hold the left trigger (L2), or slip with the stick pushed forward. A sideways weave drops that shoulder. A straight weave drops the lead shoulder |
| Lean back | Hold the left trigger (L2), or slip with the stick pulled back |

The larger stick axis wins when a slip is diagonal. Left and right are rolls. Forward is the roll under. Back is the lean.

Menus (front end, fighter select, and pause) use the D-pad or the left stick to move the red bar. W, S, and the arrow keys do the same. A and D move as well, and on Settings they change the highlighted value. Cross, Enter, or Space accepts. Circle or Escape backs out. Options pauses a bout and resumes it. With the menu closed, Cross, Circle, the shoulders, the triggers, and the right stick are the boxing inputs in the table. They are not left as menu buttons during the fight.

A solo match is you plus one opponent. Two human players do not get an AI opponent.

Mode select: on-screen menu, or console `FBMode 1` (sparring) / `2` (multiplayer) / `3` (heavy bag) / `4` (career).

Pause, during a fight or the heavy bag: Resume, Main Menu, Change Mode, Quit. The host pause freezes the local bout. It does not award a win or change damage. Take a knee uses the same referee count, and only in a live round. The heavy bag ignores it.

What each gameplay function does is listed in `docs/CodeGuide.md`.
