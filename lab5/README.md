# TECHIN515 Lab 5 - Edge-Cloud Offloading
## Discussion
- Is server's confidence always higher than wand's confidence from your observations? What is your hypothetical reason for the observation?

The server's confidence is not always higher than the wand's confidence, but it tends to be higher on average. This is likely because the server model has more computational resources and better-trained models. The local device uses a lightweight model optimized for speed and limited hardware, which may produce less confident predictions.

- Sketch the data flow of this lab.

[User Gesture] -> [Wand: Local ML Inference] -> [Confidence Threshold Check]

-> [if Confident] -> [Use Wand Output]

-> [if not Confident] -> [Send to Server for Inference] -> [Server Response Returned] -> [Use Server Output]


- Our approach is edge-first, fallback-to-server when uncertain. Analyze pros and cons of this approach from the following aspects: reliance on connectivity, latency, prediction consistency, data privacy.

| Aspect                       | Pros                                                                     | Cons                                                         |
|-----------------------------|--------------------------------------------------------------------------|--------------------------------------------------------------|
| **Reliance on Connectivity** | Can operate offline for confident predictions; less dependent on network | Still needs network access for uncertain cases               |
| **Latency**                  | Fast response for confident edge predictions                             | Higher latency when falling back to server                   |
| **Prediction Consistency**   | Local predictions give fast, real-time results in most cases             | Differences in model versions may cause inconsistent outputs |
| **Data Privacy**             | Keeps most data on-device, reducing exposure                             | Uncertain inputs still require sending data to server        |


- Name a strategy to mitigate at least one limitation named in question 3.

To mitigate prediction inconsistency, we can periodically synchronize model weights between the server and the wand. This way, both use similar models and produce more consistent results.