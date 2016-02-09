for f in Mazes/*_maze
  do 
    echo "Solving: " $f
    ./solve $f
    ./render $f
  done

for f in Mazes/*_solution
  do
    ./render $f
  done

mv Mazes/*_solution* "Naive Solutions/"
