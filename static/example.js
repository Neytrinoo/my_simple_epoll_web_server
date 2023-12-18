import '../styles/App.css';
import '../styles/excursion-page.css';
import React, {useEffect, useState} from 'react';
import {useParams} from "react-router";
import {useNavigate} from "react-router-dom";
import {useAuth} from "../core/hooks/use-auth";
import ExcursionService from "../core/services/excursions";
import {Excursion} from "../core/models/excursions";
import {Guide} from "../core/models/guides";
import {ExcursionEvent} from "../core/models/excursion_events";
import {FeedbackExcursion} from "../core/models/feedback_excursions";
import Header from "../components/common/header/header";
import GuideService from "../core/services/guides";
import ExcursionEventService from "../core/services/excursion_events";
import GuideCard from "../components/guide-card/guide-card";
import DefaultButton from "../components/common/button/default-button";
import UserService from "../core/services/users";
import ExcursionShortInfo from "../components/excursion-short-info/excursion-short-info";
import FeedbackCreate from "../components/feedback-create/feedback-create";
import FeedbackExcursionService from "../core/services/feedback_excursions";
import FeedbackList from "../components/feedback/feedback-list";

const ExcursionPage = () => {
    const params = useParams();
    const navigate = useNavigate();

    const {user, login} = useAuth();

    const [excursion, setExcursion] = useState<Excursion>();
    const [guide, setGuide] = useState<Guide>();
    const [excursionEvent, setExcursionEvent] = useState<ExcursionEvent>();
    const [isUserVisited, setIsUserVisited] = useState(false);

    const [isUserSendFeedback, setIsUserSendFeedback] = useState(false);

    const [feedbacks, setFeedbacks] = useState<FeedbackExcursion[]>();

    useEffect(() => {
        ExcursionService.getByID(Number(params.id)).then(
            (result) => {
                setExcursion(result.data);
                GuideService.getByID(result.data.guide_id).then(
                    (result) => {
                        setGuide(result.data);
                    },
                    (_) => {
                    }
                );

                // получаем события экскурсии
                ExcursionEventService.getManyByFilters({excursion_ids_in: [Number(params.id)]}).then(
                    (result) => {
                        result.data.items.sort((a: ExcursionEvent, b: ExcursionEvent) => {
                            if (b.date > a.date) {
                                return 1;
                            } else if (b.date < a.date) {
                                return -1;
                            }

                            return 0;
                        });
                        // устанавливаем в событие экскурсии последнее событие этой экскурсии
                        setExcursionEvent(result.data.items[0]);
                    },
                    (_) => {
                    }
                );
            },
            (_) => {
            }
        )
    }, [params.id]);

    useEffect(() => {
        if (user && excursion) {
            // проверяем, посещал ли пользователь экскурсию
            UserService.getUserVisitedExcursions(user.user.id).then(
                (result) => {
                    console.log('user visited =', result.data);
                    if (result.data.some((e: Excursion) => e.id === excursion.id)) {
                        console.log('set visited');
                        setIsUserVisited(true);
                    }
                },
                (_) => {
                }
            )

            // получаем отзывы экскурсии
            FeedbackExcursionService.getManyByFilters({excursion_id: Number(params.id)}).then(
                (result) => {
                    setFeedbacks(result.data.items);
                    if (result.data.items.some((f: FeedbackExcursion) => f.user.id == user.user.id)) {
                        setIsUserSendFeedback(true);
                    } else {
                        setIsUserSendFeedback(false);
                    }
                },
                (_) => {
                }
            )
        }
    }, [user, excursion]);

    // добавить запись о посещении экскурсии
    const visitExcursionOnClick = (e: React.MouseEvent<HTMLButtonElement>) => {
        if (!excursionEvent || !user) {
            return;
        }
        ExcursionEventService.addVisitor(excursionEvent.id, user.token).then(
            (result) => {
                setIsUserVisited(true);
            }
        )
    };

    // отменить запись о посещении экскурсии
    const unVisitExcursionOnClick = (e: React.MouseEvent<HTMLButtonElement>) => {
        if (!excursionEvent || !user) {
            return;
        }
        ExcursionEventService.deleteVisitor(excursionEvent.id, user.token).then(
            (result) => {
                setIsUserVisited(false);
            }
        )
    };

    const createOnClick = (rating: number, text: string) => {
        FeedbackExcursionService.create({
            excursion_id: Number(params.id),
            rating: Number(rating),
            text: text
        }, user?.token as string).then(
            (result) => {
                const newFeedbacks = [result.data, ...feedbacks ?? []];
                setFeedbacks(newFeedbacks);
                setIsUserSendFeedback(true);
            },
            (_) => {
            }
        );
    }

    return (
        <div className={"main_content"}>
            <Header></Header>
            <div className="excursion_page_content">
                <div className="excursion_info">
                    <div className="excursion_info__img_and_short_info">
                        <img src={excursion?.url_to_avatar} alt="" className="img_and_short_info__img"/>
                        <div className="img_and_short_info__short_info">
                            <GuideCard guide={guide}/>
                            {excursion && excursionEvent &&
                                <ExcursionShortInfo excursion={excursion as Excursion}
                                                    excursionEvent={excursionEvent as ExcursionEvent}/>
                            }

                            {user && excursionEvent && isUserVisited &&
                                <DefaultButton text={'Отменить запись'} onClick={unVisitExcursionOnClick}
                                               isAccentColor={true}/>
                            }
                            {user && excursionEvent && !isUserVisited &&
                                <DefaultButton text={'Записаться на экскурсию'} onClick={visitExcursionOnClick}/>
                            }
                        </div>
                    </div>
                    <div className="excursion_info__title_and_about">
                        <p className="title2_font">{excursion?.title}</p>
                        <p className="main_text_font excursion_info__about">{excursion?.long_description}</p>
                    </div>
                </div>
                <div className="feedback_excursions">
                    <div className="feedback_excursions__create">
                        <p className={'title1_font'}>Отзывы</p>
                        {!isUserSendFeedback && user && <FeedbackCreate createOnClick={createOnClick}/>}
                    </div>
                    <div className="feedback_excursions__feedbacks">
                        <FeedbackList feedbacks={feedbacks as FeedbackExcursion[]}/>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default ExcursionPage;
