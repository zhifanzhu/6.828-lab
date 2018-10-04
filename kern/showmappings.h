#ifndef JOS_KERN_SHOWMAPPINGS_H
#define JOS_KERN_SHOWMAPPINGS_H


struct Trapframe;

int mon_showmappings(int argc, char **argv, struct Trapframe *tf);

#endif	// !JOS_KERN_SHOWMAPPINGS_H
