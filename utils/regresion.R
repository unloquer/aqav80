rm(list=ls())

## https://www.szynalski.com/tone-generator/
## 1 -> sonido ambiente
## 2 -> 1%
## 3 -> 5%
## 4 -> 7%
## 5 -> 10%
## 6 -> 13%
## 7 -> 18%
## 8 -> 25%
## 9 -> 35%
## 10 -> 50%
## 11 -> 60%
## 12 -> 70%
## 13 -> 80%
## 14 -> 90%

mean(c(805,805,805,805,805,805,805,805,805,805,805,805,805,805,805,805,805,805,805,805))
micAnalogValues <- c(57.83871,150.2549,481.2222,493,496.7333,498.2963,499.2414,499.4583,666.2667,745.8235,804.8276,805)
meterDbs <- c(32,39.2,53.1,55.5,58.8,66.3,68.8,71.2,77.7,78.5,78.9,79.2)

scatter.smooth(x=meterDbs, y=micAnalogValues, main="Dist ~ Speed")

cor(meterDbs, micAnalogValues)
lm(meterDbs ~ micAnalogValues)

dbs <- 29.71793 + 0.06495*micAnalogValues

scatter.smooth(x=dbs, y=meterDbs, main="Dist ~ Speed")

## Call:
##     lm(formula = meterDbs ~ micAnalogValues)

## Coefficients:
##     (Intercept)  micAnalogValues  
## 29.71793          0.06495  

d <- ## output de consola serial
plot(d, type =  'l')
