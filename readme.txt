This repo contains the original source code for the Vischeck/Tinyeyes application - mostly written by Robert Dougherty with some input from Alex Wade and Heidi Baseler. 

Typically this ran as a binary on a web server and converted uploaded images into colour blind and/or infant vision simulations. But you can also (conveniently) run it on any linuxy command line. 

I have ensured that the most recent version of this code compiles on >my< machine (a 2021 OSX M1 MacBook) but it should be pretty much fine on anything. 

make clean
make -f MakefileMac
 
Requires: On OSX you will use Homebrew (not the Apple g++) . You also want to brew install fftw3. You might find some other dependencies as you go - but brew can take care of all of them!

An example of how to call the code is in test.pl and you can also use
 ./runVischeck3 --help

To get the args. 

Some version of this code was included into the GIMP in the mid-00s - you might be able to find it there as well. And there was once a java version (a plugin for imageJ) which is currently MIA.


ARW Oct 20/2021

Thanks to the open source community and especially Jama, fftw and the compiler maintainers - having this compile almost seamlessly after 30 years is great!
