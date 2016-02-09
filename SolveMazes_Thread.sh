for f in Mazes/*_maze
  do 
    echo "Solving: " $f
    ./solve $f -t
  done

for f in Mazes/*_solution
  do
    ./render $f
  done

mv Mazes/*_solution* "Threaded Solutions/"
