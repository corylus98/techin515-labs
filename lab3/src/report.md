# TECHIN 515 Lab3
### Video
See `video.MOV` for the working sorting hat prototype.
### Source code
See `/lab3/src/main.cpp` for the source code.
### Discussion
1. Play with your sorting hat. Are all 10 questions important to create the sorting hat? If you were to remove some questions to improve user experience, which questions would you remove and justify your answer.

Not all 10 questions are important to get the result. If I were to remove some of them, I would extract feature importance scores from the decision tree, and remove the ones with lower importance. After that, I would retrain the model to evaluate the accuracy, ensuring that the features removed are valid. 

2. If you were to improve the sorting hat, what technical improvements would you make? Consider:
    - How could you improve the model's accuracy or efficiency?
    - What additional sensors or hardware could enhance the user experience?
    - Does decision tree remain suitable for your choice of new sensors? If yes, carefully justify your answer. If not, what ML model would you use and explain why.

I think the current verison is quite efficient, so I might try to improve the accuracy with other models, such as gradient boosting. For user experience, I would allow users to speak answers instead of pressing buttons. Accordingly, I would need some STT models to translate users' speech into answer. But after this process, I could use the decision tree model to get the classification task done.