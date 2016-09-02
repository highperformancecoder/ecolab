# ecolab

EcoLab is both the name of a software package and a research project that is looking at the dynamics of evolution.

EcoLab the software package is now a fairly complete agent-based simulation system, with the following features:

*    The model is implemented as a C++ object. Support for more advanced data structures and algorithms are available through C++'s standard library.
*    A scripting language TCL can access the model's methods and instance variables, allowing experiments to be set up dynamically at runtime
*    The model can be run in GUI mode through the use of Tk and BLT widgets. Various graphs and histogramming tools are available for use.
*    The same model can be run in batch mode by using a different script
*    The model can be probed dynamically with the object browser
*    The model can be saved to a checkpoint file, or transferred periodically over a socket connection to another copy of Ecolab acting as a visualisation client.
*    Various types of random number generators are available through the UNURAN library or through the GNUSL.
*    A form of SPMD parallel programming is provided through ClassdescMP.
*    EcoLab models may also use Graphcode library to implement a distributed network of agents over an MPI-based cluster computer.

Documentation, tickets and file releases are hosted on [SourceForge](http://ecolab.sf.net)

The official EcoLab code repository is hosted here.

### EcoLab ABM framework publications

Please use reference 7 to cite EcoLab.

1.   Standish R.K. (2012) ``Complexity of Networks (reprise)'', Complexity, 17, 50-61 arXiv: 0911.3482

2.    Standish R.K. (2010) ``Network Complexity of Foodwebs", Proceedings Artificial Life XII, Fellermann et al. (eds), (MIT Press: Cambridge, MA) 337-343. arXiv: 1008.3800

3.    Standish, R.K. and Madina, D. (2008) ``Classdesc and Graphcode: support for scientific programming in C++'', arXiv:cs.CE/0610120

4.    Standish, R.K. (2008) ``Going Stupid with EcoLab'', Simulation, 84, 611-618. arXiv: cs.MA/0612014

5.    Standish, R.K. (2008) ``Open Source Agent-based Modelling Frameworks'', Studies in Computational Intelligence, 115, 409-437. ©Springer-Verlag.

6.    Standish, R.K. and Madina, D. (2003) ``ClassdescMP: Easy MPI programming in C++'' in Computational Science, Sloot et al. (eds), LNCS 2660, Springer, 896. arXiv:cs.DC/0401027

7.    Standish, R.K. and Leow, R. (2003) ``EcoLab: Agent Based Modeling for C++ programmers'', in Proceedings SwarmFest 2003. arXiv:cs.MA/0401026

8.    Leow, R. and Standish, R.K. (2003) ``Running C++ models under the Swarm Environment'', in Proceedings SwarmFest 2003. arXiv:cs.MA/0401025

9.    Madina, D. and Standish, R.K. (2001) ``A system for reflection in C++'', in Proceedings AUUG 2001: Always on and Everywhere, 207. ISBN 0957753225 arXiv:cs.PL/0401024

10.    Standish, R.K. (2000) ``Ecolab 4'', in Applied Complexity: From Neural Networks to Managed Landscapes Halloy, S. and Williams, T. (eds), (New Zealand Institute for Crop and Food Research, Christchurch), 156-163.

### EcoLab model publications

1.    Standish, R.K. (2004) ``Ecolab, Webworld and self-organisation'', in Artificial Life IX, Pollack, J. et al., (Cambridge, MA: MIT Press), 358-363. arXiv:nlin.AO/0404011.

2.    Standish, R.K. (2002) ``Diversity Evolution'' in Artificial Life VIII, Standish, RK, Bedau, MA and Abbass, HA (eds) (Cambridge, MA: MIT Press), 131-137. arXiv:nlin.AO/0210026

3.    Standish, R.K. (2000) ``An Ecolab Perspective on the Bedau Evolutionary Statistics'', in Proceedings Artificial Life VII, Bedau, M.A. et al. (eds), (MIT Press: Cambridge, Mass.), 238-242. arXiv:nlin.AO/0004026

4.    Standish, R.K. (2000) ``The Role of Innovation within Economics'', in Commerce, Complexity and Evolution, Barnett, W. et al (eds) (Cambridge University Press, New York), pp61-79. arXiv:nlin.AO/0007005

5.    Standish, R.K. (1999) ``Statistics of Certain Models of Evolution'', Phys. Rev. E, 59 1545-1550. arXiv:physics/9806046

6.    Standish, R.K. (1998) ``Cellular Ecolab'', in Complex Systems '98, Standish et. al. (eds) (Complexity Online) and and Complexity International , 6.

7.    Standish, R.K. (1996) ``Ecolab: Where to now?'' in Complex Systems: From Local Interactions to Global Phenomena, R. Stocker, H. Jelinek, B. Durnota and T. Bossomeier (IOS: Amsterdam) 1996, p262-271. also Complexity International , 3.

8.    Standish, R.K. (1994) ``Population models with Random Embryologies as a Paradigm for Evolution'', in Complex Systems: Mechanism of Adaption, R.J. Stonier and X.H. Yu (eds), p45-51 (IOS:Amsterdam). also Complexity International , 2.

