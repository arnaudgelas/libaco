#include <map>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <exception>
#include <map>
#include <list>
#include <libaco/ants.h>
#include <liblocalsearch/localsearch.h>

template <class T> class EliminationGraph : public T {
  public:
    EliminationGraph(const T &graph) : T(graph) {
    }

    ~EliminationGraph() {
    }

    void eliminate(unsigned int vertex) {
      std::vector<unsigned int> neighbours = T::get_neighbours(vertex);
      for(int i=0;i<neighbours.size();i++) {
        for(int j=i;j<neighbours.size();j++) {
          if(i!=j) {
            T::add_edge(neighbours[i],neighbours[j]);
          }
        }
        T::remove_edge(vertex,neighbours[i]);
      }
    }

    T &operator=(const T &graph) {
      T::operator=(graph);
      return *this;
    }
};

typedef double (*heuristicf)(const Graph &graph, unsigned int vertex);

namespace Heuristic {
  double min_degree(const Graph &graph, unsigned int vertex);
  double min_fill(const Graph &graph, unsigned int vertex);
}

template <class T> std::vector<unsigned int> get_max_clique_positions(const T &graph, const std::vector<unsigned int> &solution) {
  unsigned int max_clique = 0;
  std::vector<unsigned int> max_clique_positions;
  EliminationGraph<T> elim_graph(graph);
  for(unsigned int i=0;i<solution.size();i++) {
    unsigned int w = elim_graph.get_neighbours(solution[i]).size();

    if(w == max_clique) {
      max_clique_positions.push_back(i);
    } else if(max_clique < w) {
      max_clique_positions.clear();
      max_clique_positions.push_back(i);
      max_clique = w;
    } 

    elim_graph.eliminate(solution[i]);
    if(solution.size()-i <= max_clique) {
      break;
    }
  }
  return max_clique_positions;
}

template <class T> class MaxCliqueNeighbourhood : public Neighbourhood {
  private:
    T *graph_;
    std::vector<unsigned int> solution_;
    std::vector<unsigned int> neighbour_;
    bool has_next_neighbour_;
  public:
    MaxCliqueNeighbourhood(const T &graph, std::vector<unsigned int> solution) {
      graph_ = new T(graph);
      solution_ = solution;
      neighbour_ = solution;
      has_next_neighbour_ = true;
    }

    ~MaxCliqueNeighbourhood() {
      delete graph_;
    }

    void set_solution(std::vector<unsigned int> solution) {
      solution_ = solution;
      neighbour_ = solution;
      has_next_neighbour_ = true;
    }

    std::vector<unsigned int> get_solution() {
      return solution_;
    }

    bool has_next_neighbour_solution() {
      return has_next_neighbour_;
    }

    const std::vector<unsigned int> &next_neighbour_solution() {
      std::vector<unsigned int> max_cliques = get_max_clique_positions<T>(*graph_, solution_);
      unsigned int max_clique_pos = 0;
      unsigned int other_pos = 0;
      while(max_clique_pos == other_pos) {
        max_clique_pos = max_cliques[random_number(max_cliques.size())];
        other_pos = random_number(solution_.size());
      }

      unsigned int tmp = neighbour_[max_clique_pos];
      neighbour_[max_clique_pos] = neighbour_[other_pos];
      neighbour_[other_pos] = tmp;
      has_next_neighbour_ = false;
      return neighbour_;
    }
};

template <class T> class DecompProblem : public OptimizationProblem, public EvaluationFunction, public PerturbationFunction {
  protected:
    T *graph_;
    EliminationGraph<T> *elim_graph_;
    std::map<unsigned int,bool> visited_vertices_;
    std::vector<double> vertex_weight_;
    heuristicf heuristic_; 
    unsigned int vertices_eliminated_;
  public:
    DecompProblem(T *graph, heuristicf heuristic=Heuristic::min_degree) {
      graph_ = graph;
      elim_graph_ = new EliminationGraph<T>(*graph);
      vertex_weight_.reserve(graph->number_of_vertices());
      heuristic_ = heuristic;
      vertices_eliminated_ = 0;
    }

    ~DecompProblem() {
      delete graph_;
      delete elim_graph_;
    }

    unsigned int get_max_tour_size() {
      return graph_->number_of_vertices();
    }

    unsigned int number_of_vertices() {
      return graph_->number_of_vertices();
    }

    virtual std::map<unsigned int,double> get_feasible_start_vertices() {
      std::map<unsigned int,double> vertices;
      for(unsigned int i=0;i<graph_->number_of_vertices();i++) {
        vertices[i] = heuristic_(*graph_, i);
      }
      return vertices;
    }

    virtual std::map<unsigned int,double> get_feasible_neighbours(unsigned int vertex) {
      std::map<unsigned int,double> vertices;
      for(unsigned int i=0;i<graph_->number_of_vertices();i++) {
        if (!visited_vertices_[i]) {
          vertices[i] = heuristic_(*elim_graph_, i);
        }
      }
      return vertices;
    }

    double eval_tour(const std::vector<unsigned int> &tour) {
      double width = compute_width(tour);
      return width;
    }

    double pheromone_update(unsigned int v, double tour_length) {
      return 1.0 / (vertex_weight_[v] + 1.0) * (1.0 / tour_length);
    }

    void added_vertex_to_tour(unsigned int vertex) {
      vertex_weight_[vertex] = elim_graph_->get_neighbours(vertex).size() * 1.0 / (elim_graph_->number_of_vertices() - vertices_eliminated_);
      elim_graph_->eliminate(vertex);
      vertices_eliminated_++;
      visited_vertices_[vertex] = true;
    }

    bool is_tour_complete(const std::vector<unsigned int> &tour) {
      return tour.size() == graph_->number_of_vertices();
    }

    double eval_solution(const std::vector<unsigned int> &solution) {
      return 1.0 / eval_tour(solution);
    }

    std::vector<unsigned int> perturbate(const std::vector<unsigned int> &solution) {
      std::vector<unsigned int> new_solution = solution;
      std::vector<unsigned int> max_cliques = get_max_clique_positions<T>(*graph_, solution);
      for(unsigned int i=0;i<max_cliques.size();i++) {
        unsigned int swap_pos = random_number(new_solution.size());
        unsigned int tmp = new_solution[max_cliques[i]];
        new_solution[max_cliques[i]] = new_solution[swap_pos];
        new_solution[swap_pos] = tmp;
      }
      return new_solution;
    }

    std::vector<unsigned int> apply_local_search(const std::vector<unsigned int> &tour) {
      MaxCliqueNeighbourhood<T> neighbourhood(*graph_, tour);
      HillClimbing climbing(tour, *this, neighbourhood);
      IterativeLocalSearch search(&climbing, this);
      search.run(100);
      return climbing.get_best_so_far_solution();
    }

    void cleanup() {
      vertices_eliminated_ = 0;
      *elim_graph_ = *graph_;
      visited_vertices_.clear();
    }

    virtual unsigned int compute_width(const std::vector<unsigned int> &tour) = 0;
};

template <class T> class TreeDecompProblem : public DecompProblem<T> {
  public:
    TreeDecompProblem(T *graph, heuristicf heuristic=Heuristic::min_degree) : DecompProblem<T>(graph, heuristic) {
    }

    unsigned int compute_width(const std::vector<unsigned int> &tour) {
      unsigned int width = 0;
      EliminationGraph<T> graph(*DecompProblem<T>::graph_);
      for(unsigned int i=0;i<tour.size();i++) {
        unsigned int w = graph.get_neighbours(tour[i]).size();
        graph.eliminate(tour[i]);
        if(w > width) {
          width = w;
        }

        if(tour.size()-i <= width) {
          break;
        }
      }
      return width;
    }
};

template <class T> class HyperTreeDecompProblem : public DecompProblem<T> {
  private:
    HyperGraph *hypergraph_;
  public:
    HyperTreeDecompProblem(HyperGraph *hypergraph, heuristicf heuristic=Heuristic::min_degree) : DecompProblem<T>(&hypergraph->get_primal_graph<T>(), heuristic) {
      hypergraph_ = hypergraph;
    }

    ~HyperTreeDecompProblem() {
      delete hypergraph_;
    }

    unsigned int compute_width(const std::vector<unsigned int> &tour) {
      unsigned int width = 0;
      EliminationGraph<T> graph(*DecompProblem<T>::graph_);
      for(unsigned int i=0;i<tour.size();i++) {
        std::vector<unsigned int> vertices = graph.get_neighbours(tour[i]);
        vertices.push_back(tour[i]);
        // set covering
        unsigned int w = compute_greedy_hyperedge_covering(vertices).size();
        graph.eliminate(tour[i]);
        if(w > width) {
          width = w;
        }
      }
      return width;
    }

    std::vector<unsigned int> compute_greedy_hyperedge_covering(std::vector<unsigned int> vertices) {
      std::vector<unsigned int> edges;
      std::map< unsigned int, std::vector<unsigned int> > edges_vertices;
      while(!vertices.empty()) {
        for(unsigned int i=0;i<vertices.size();i++) {
          unsigned int vertex = vertices[i];
          std::vector<unsigned int> vertex_edges = hypergraph_->get_edges_for_vertex(vertex);
          for(unsigned int j=0;j<vertex_edges.size();j++) {
            edges_vertices[vertex_edges[j]].push_back(vertex);
          }
        }

        //get edge that covers a maximum of vertices
        unsigned int maximum_edge;
        std::vector<unsigned int> vertices_covered;
        for(std::map< unsigned int, std::vector<unsigned int> >::iterator it=edges_vertices.begin();it!=edges_vertices.end();it++) {
          if((*it).second.size() > vertices_covered.size()) {
            maximum_edge = (*it).first;
            vertices_covered = (*it).second;
          }
        }

        //remove vertices covered by edge from vertices
        for(unsigned int k=0;k<vertices_covered.size();k++) {
          for(std::vector<unsigned int>::iterator it2=vertices.begin();it2!=vertices.end();it2++) {
            if(*it2 == vertices_covered[k]) {
              vertices.erase(it2);
              break;
            }
          }
        }

        edges.push_back(maximum_edge);
      }
      return edges;
    }

    std::map<unsigned int,double> get_feasible_start_vertices() {
      std::map<unsigned int,double> vertices;
      for(unsigned int i=0;i<DecompProblem<T>::graph_->number_of_vertices();i++) {
        std::vector<unsigned int> clique = DecompProblem<T>::graph_->get_neighbours(i);
        clique.push_back(i);
        vertices[i] = 1.0 / compute_greedy_hyperedge_covering(clique).size();
      }
      return vertices;
    }

    std::map<unsigned int,double> get_feasible_neighbours(unsigned int vertex) {
      std::map<unsigned int,double> vertices;
      for(unsigned int i=0;i<DecompProblem<T>::graph_->number_of_vertices();i++) {
        if (!DecompProblem<T>::visited_vertices_[i]) {
          std::vector<unsigned int> clique = DecompProblem<T>::graph_->get_neighbours(i);
          clique.push_back(i);
          vertices[i] = 1.0 / compute_greedy_hyperedge_covering(clique).size();
        }
      }
      return vertices;
    }
};

class FileNotFoundException : public std::exception {
  private:
    const char *filepath_;
  public: 
    FileNotFoundException(const char *filepath);
    const char *what() const throw();
};

namespace Parser {
  template <class T> Graph &parse_dimacs(const char *filepath) throw(FileNotFoundException) {
    Graph *graph;
    int number_of_vertices;
    int vertex_a, vertex_b;
    char problem[5];
    char buf[1024];
    char flag;
    std::ifstream file(filepath);

    if(!file) {
      throw FileNotFoundException(filepath);
    }

    while(file.good()) {
      file >> flag;
      switch(flag) {
        case 'p':
          file >> problem;
          if (!strcmp(problem, "edge")) {
            file >> number_of_vertices;
            graph = new T(number_of_vertices);
          }
          break;
        case 'e':
          file >> vertex_a;
          file >> vertex_b;
          graph->add_edge(vertex_a-1, vertex_b-1);
          break;
        default:
          file.getline(buf, 1024);
          break;
      }
    }
    file.close();
    return *graph;
  }

  HyperGraph &parse_hypertreelib(const char *filepath) throw(FileNotFoundException);
}
